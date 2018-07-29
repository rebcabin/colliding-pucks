from pygame.color import THECOLORS

import pymunk
from pymunk.pygame_util import draw

import pprint
pp = pprint.PrettyPrinter(indent=2)

import globals
from tw_types import *
from rendering import *
from funcyard import *

# TODO: IDs might be best as uuids.
# TODO: Message equality can be optimized.


#  _____     _    _       ___          _
# |_   _|_ _| |__| |___  | _ \___ __ _(_)___ _ _
#   | |/ _` | '_ \ / -_) |   / -_) _` | / _ \ ' \
#   |_|\__,_|_.__/_\___| |_|_\___\__, |_\___/_||_|
#                                |___/


class TableRegion(LogicalProcess):
    """TODO"""


# __      __    _ _
# \ \    / /_ _| | |
#  \ \/\/ / _` | | |
#   \_/\_/\__,_|_|_|


class Wall(object):

    def __init__(self,
                 left: Vec2d,
                 right: Vec2d,
                 color=THECOLORS['yellow']):
        self.left = left
        self.right = right
        self.color = color

    def draw(self):
        draw_int_tuples([self.left, self.right], self.color)


class WallLP(LogicalProcess):

    def __init__(self, wall, me: ProcessID):
        super().__init__(me)
        self.wall = wall

    def draw(self):
        self.wall.draw()


#  ___         _
# | _ \_  _ __| |__
# |  _/ || / _| / /
# |_|  \_,_\__|_\_\


class Puck(object):
    """Double duty with time warp and standard sim."""
    def __init__(self, center, velocity, mass, radius, color, dont_fill_bit=0):

        self._original_center = center
        self._original_velocity = velocity

        # instance variables: need them in the tw-state
        self.center = center
        self.velocity = velocity

        # instance constants; don't need them in the tw-state
        self.MASS = mass
        self.RADIUS = radius
        self.COLOR = color
        self.DONT_FILL_BIT = dont_fill_bit

    def reset(self):
        self.center = self._original_center
        self.velocity = self._original_velocity

    def step(self, dt: float):
        """not used yet"""
        raise NotImplementedError
        self.center += dt * self.velocity

    def draw(self):
        pygame.draw.circle(globals.screen,
                           self.COLOR,
                           self.center.int_tuple,
                           self.RADIUS,
                           self.DONT_FILL_BIT)

    def step_many(self, steps, dt: float):
        self.center += steps * dt * self.velocity

    def predict_a_wall_collision(self, wall: Wall, dt):
        c = self.center
        q, t = collinear_point_and_parameter(wall.left, wall.right, c)
        n = (q - c).normalized()
        v_n = self.velocity.dot(n)
        t = Vec2d(n[1], -n[0])
        v_t = self.velocity.dot(t)
        p_c = c + self.RADIUS * n
        q_prime, t_prime = collinear_point_and_parameter(
            wall.left, wall.right, p_c)
        # TODO: q_prime should be almost the same as q
        # TODO: np.testing.assert_allclose(...), meanwhile, inspect in debugger.
        d = (q_prime - p_c).length
        # Predicted step time can be negative! It is permitted!
        # d / v_n is in physical time units. Tau is in units of steps,
        # where 1/dt is the physical time of one step.
        tau_physical = d / v_n
        tau = tau_physical / dt if v_n != 0 else np.inf
        c_prime = c + tau_physical * self.velocity
        # Reverse the normal component:
        v_prime = v_t * t - v_n * n
        k, p = self.puck_energy_momentum(self.velocity)
        k_prime, p_prime = self.puck_energy_momentum(v_prime)
        return {'tau': int(tau),
                'tau_physical': tau_physical,
                "delta k": k - k_prime,
                "delta p": (p - p_prime).length,
                'puck_strike_point': p_c,
                'wall_strike_point': q_prime,
                'wall_strike_parameter': t_prime,
                'wall_victim': wall,
                "c'": c_prime,
                "v'": v_prime}

    def puck_energy_momentum(self, v):
        p = self.MASS * v
        k = p.dot(p) / 2 / self.MASS
        return (k, p)

    def predict_a_puck_collision(self, them: 'Puck', dt):
        """See https://goo.gl/jQik91 for forward-references as strings."""
        dp = them.center - self.center
        # Relative distance as a function of time, find its zero:
        #
        # Collect[{x-vx t, y-vy t}^2 - d1^2, t]
        #
        # (x^2+y^2)-d1^2 + t (-2 x vx-2 y vy) + t^2 (vx^2 + vy^2)
        # \_____ ______/     \______ _______/       \_____ _____/
        #       v                   v                     v
        #
        #       c                   b                     a
        #
        dv = self.velocity - them.velocity
        # a, b, c are the coefficients of the quadratic above, in the usual
        # notation we all know from high-school algebra.
        a = dv.get_length_sqrd()
        b = -2 * dp.dot(dv)
        d1 = self.RADIUS + them.RADIUS
        c = dp.get_length_sqrd() - (d1 * d1)
        disc = (b * b) - (4 * a * c)
        gonna_hit = False
        tau = tau_physical = np.inf
        if disc >= 0 and a != 0:
            # Two real roots; pick the smallest, non-negative one.
            sdisc = np.sqrt(disc)
            root1 = (-b + sdisc) / (2 * a)
            root2 = (-b - sdisc) / (2 * a)
            # The roots are in units of physical time. Tau is in units of
            # steps, where 1/dt is the physical time of one step.
            if root1 > 0 and root2 > 0:
                tau_physical = float(min(root1, root2))
                tau = tau_physical / dt
                gonna_hit = True
            else:
                tau_physical = float(max(root1, root2))
                tau = tau_physical / dt
                gonna_hit = root1 > 0 or root2 > 0
            # TODO: what if they're both negative? Is that possible?

        c1_prime = None
        c2_prime = None
        v1_prime = None
        v2_prime = None
        k = None
        k_prime = None
        p = None
        p_prime = None
        if gonna_hit:
            assert tau != np.inf
            assert tau > 0
            v1 = self.velocity
            v2 = them.velocity
            k, p = self.puck_puck_energy_momentum(them, v1, v2)
            c1_prime = self.center + tau_physical * v1
            c2_prime = them.center + tau_physical * v2
            normal = (c2_prime - c1_prime).normalized()
            tangential = Vec2d(normal[1], -normal[0])
            v1n = v1.dot(normal)
            v2n = v2.dot(normal)
            v1t = v1.dot(tangential)
            v2t = v2.dot(tangential)
            m1 = self.MASS
            m2 = them.MASS
            M = m1 + m2
            # See Mathematica notebook in docs folder
            v1np = ((m1 - m2) * v1n + 2 * m2 * v2n) / M
            v2np = ((m2 - m1) * v2n + 2 * m1 * v1n) / M
            v1_prime = v1np * normal + v1t * tangential
            v2_prime = v2np * normal + v2t * tangential
            k_prime, p_prime = self.puck_puck_energy_momentum(them, v1_prime, v2_prime)
        return {
            'tau': int(tau) if tau < np.inf else sys.maxsize,
            'tau_physical': tau_physical,
            "delta k": k - k_prime if k_prime else 0,
            "delta p": (p - p_prime).length if p_prime else 0,
            'puck_victim': them,
            'gonna_hit': gonna_hit,
            "c1'": c1_prime,  # TODO: can't index later with string "c1'"
            "v1'": v1_prime,
            "c2'": c2_prime,
            "v2'": v2_prime}

    def puck_puck_energy_momentum(self, them, v1, v2):
        # momentum
        p1 = self.MASS * v1
        p2 = them.MASS * v2
        p = p1 + p2
        # energy
        k1 = p1.dot(p1) / 2 / self.MASS
        k2 = p2.dot(p2) / 2 / them.MASS
        k = k1 + k2
        return (k, p)


class PuckLP(LogicalProcess):
    """TODO: Free logging with the oslash Writer monad."""
    def event_main(self, lvt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        self.vt = lvt
        assert lvt == self.now

        # Move the puck physics object to the commanded state:
        self.puck.center = state.body['center']
        self.puck.velocity = state.body['velocity']

        # The puck may move to table sectors with different walls,
        # so the list of walls must be in the tw-state. Likewise,
        # some collision schemes change dt, so dt is in the state.
        walls = state.body['walls']
        dt = state.body['dt']

        # Priority to 'suffer' over 'move' or anything else.
        msg = msgs[0]
        for temp in msgs:
            if temp.body['action'] == 'suffer':
                msg = temp

        # We've picked one, now process it:
        if msg.body['action'] == 'suffer':

            # Go where I'm told
            state_prime = self.new_state(Body({
                'center': msg.body['center'],
                'velocity': msg.body['velocity'],
                'walls': walls,
                'dt': dt}))

            # Move a tiny bit (TODO: risky; subdivide time)
            self.send(rcvr_pid=self.me,
                      receive_time=self.now + 1,
                      body=Body({'action': 'move'}))

        elif msg.body['action'] == 'move':
            wall_pred = wall_prediction(self.puck, walls, dt)
            tau_wall = wall_pred['tau']
            it, its_lp = self._get_other()
            puck_pred = self.puck.predict_a_puck_collision(it, dt)
            tau_puck = puck_pred['tau']
            # TODO: Can't handle strikes at time 'now'
            if puck_pred['gonna_hit'] and 0 < tau_puck < tau_wall:
                pp.pprint({'coll_pred': self.me,
                           'with': its_lp.me,
                           'tau': tau_puck,
                           'pred_lvt': self.now + tau_puck,
                           'now': self.now})
                # Check energy and momentum conservation.
                assert np.abs(puck_pred["delta k"]) < 1e-12
                assert puck_pred["delta p"] < 1e-12
                state_prime = self._bounce_pucks(  # sending happens in here
                    state, puck_pred, its_lp, tau_puck, walls, dt)
            else:
                pp.pprint({'coll_pred': self.me,
                           'with': 'wall',  # TODO: get the 'me' names
                           'tau': tau_wall,
                           'pred_lvt': self.now + tau_wall or 1,
                           'now': self.now})
                # Check energy conservation.
                assert np.abs(wall_pred["delta k"]) < 1e-12
                # Wall collision does not preserve puck momentum.
                # assert wall_pred["delta p"] < 1e-12
                # print({'wall dt': wall_pred['tau']})
                state_prime = self._bounce_off_wall(  # sending in here
                    state, wall_pred, walls, dt)

        else:
            raise ValueError(f'unknown message action & body '
                             f'{pp.pformat(msg)}')

        return state_prime

    def _get_other(self):
        if self.me == 'small puck':
            it, its_tw_state = self.query('big puck', Body({}))
            its_lp = globals.world_map['big puck']
        else:
            it, its_tw_state = self.query('small puck', Body({}))
            its_lp = globals.world_map['small puck']
        return it, its_lp

    def _visualize_puck(self, state, state_prime):
        """Temporary method for debugging collisions. Aso of Tue, 24 July 2018,
        I'm convinced the collision geometry is correct."""
        c = state.body['center']
        v = state.body['velocity'] * 100
        c_prime = state_prime.body['center']
        v_prime = state_prime.body['velocity'] * 100
        pygame.draw.circle(
            globals.screen,
            self.puck.COLOR,
            state_prime.body['center'].int_tuple,
            self.puck.RADIUS,
            self.puck.DONT_FILL_BIT
        )
        pygame.draw.circle(
            globals.screen,
            THECOLORS['black'],
            state_prime.body['center'].int_tuple,
            self.puck.RADIUS,
            True  # don't fill
        )
        draw_vector(c_prime, c_prime + v, THECOLORS['gray60'])
        draw_vector(c_prime, c_prime + v_prime, THECOLORS['magenta'])
        pygame.display.flip()
        time.sleep(0)

    def _bounce_pucks(self,
                      state: State,
                      puck_pred: Dict,
                      other_lp: 'PuckLP',
                      then: VirtualTime,
                      walls: List[Wall],
                      dt: float):
        state_prime = self.new_state(Body({
            'center': puck_pred["c1'"],
            'velocity': puck_pred["v1'"],
            'walls': walls,
            'dt': dt}))
        self._visualize_puck(state, state_prime)
        self.send(rcvr_pid=self.me,
                  receive_time=self.now + then,
                  body=Body({'action': 'move'}))
        self.send(rcvr_pid=other_lp.me,
                  receive_time=self.now + then,
                  body=Body({'action': 'suffer',
                             'center': puck_pred["c2'"],
                             'velocity': puck_pred["v2'"]}))
        return state_prime

    def _bounce_off_wall(self, state, wall_pred, walls, dt):
        then = int(wall_pred['tau']) or 1
        state_prime = self.new_state(Body({
            'center': wall_pred["c'"],
            'velocity': wall_pred["v'"],
            'walls': walls,
            'dt': dt}))
        self._visualize_puck(state, state_prime)
        self.send(rcvr_pid=self.me,
                  receive_time=self.now + then,
                  body=Body({'action': 'move'}))
        return state_prime

    def query_main(self, vt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        """Someone is asking for my latest earlier known state."""
        # TODO: Return a query-response message.
        return self.puck, state

    def __init__(self, puck, me: ProcessID):
        super().__init__(me)
        self.puck = puck

    def draw(self):
        self.puck.draw()


#  ___
# |   \ ___ _ __  ___ ___
# | |) / -_) '  \/ _ (_-<
# |___/\___|_|_|_\___/__/


def demo_cage_time_warp(drawing=True, pause=0.75, dt=1):
    """"""
    clear_screen()
    # TODO: Drawing should happen as side effect of first event messages

    wall_lps = mk_walls()
    [w.draw() for w in wall_lps]
    walls = [w.wall for w in wall_lps]

    small_puck_lp = mk_small_puck(walls, dt)
    small_puck_lp.draw()
    draw_centered_arrow(small_puck_lp.puck.center,
                        small_puck_lp.puck.velocity)

    big_puck_lp = mk_big_puck(walls, dt)
    big_puck_lp.draw()
    draw_centered_arrow(big_puck_lp.puck.center,
                        big_puck_lp.puck.velocity)

    # boot the OS
    for wall in wall_lps:
        globals.sched_q.insert(wall)
    globals.sched_q.insert(small_puck_lp)
    globals.sched_q.insert(big_puck_lp)

    # boot the simulation:
    small_puck_lp.send(
        rcvr_pid=ProcessID('small puck'),
        receive_time=VirtualTime(0),
        body=Body({
            'action': 'move'
        }),
        force_send_time=EARLIEST_VT)
    print({'sending': 'BOOT move',
           'receiver': 'small puck',
           'send time': EARLIEST_VT,
           'receive_time': 0})

    big_puck_lp.send(
        rcvr_pid=ProcessID('big puck'),
        receive_time=VirtualTime(0),
        body=Body({
            'action': 'move'
        }),
        force_send_time=EARLIEST_VT)
    print({'sending': 'BOOT move',
           'receiver': "big puck",
           'send time': EARLIEST_VT,
           'receive_time': 0})

    globals.sched_q.run(drawing=drawing, pause=pause)

    pygame.display.flip()
    time.sleep(pause)


def mk_walls():
    wall_lps = [WallLP(Wall(SCREEN_TL, SCREEN_BL), "left wall"),
                WallLP(Wall(SCREEN_BL, SCREEN_BR), "bottom wall"),
                WallLP(Wall(SCREEN_BR, SCREEN_TR), "right wall"),
                WallLP(Wall(SCREEN_TR, SCREEN_TL), "top wall")]
    return wall_lps


def mk_puck(center, velocity, mass, radius, color_str, pid, walls, dt):
    state = State(
        sender=pid,
        send_time=EARLIEST_VT,
        body=Body({'center': center,
                   'velocity': velocity,
                   'walls': walls,
                   'dt': dt}))
    lp = PuckLP(
        puck=Puck(
            center=center,
            velocity=velocity,
            mass=mass,
            radius=radius,
            color=THECOLORS[color_str]),
        me=pid)
    # TODO: initial state should be in this constructor
    lp.sq.insert(state)
    return lp


def mk_small_puck(walls, dt):
    return mk_puck(
        center=Vec2d(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2),
        velocity=Vec2d(2.3, -1.7),
        mass=100,
        radius=42,
        color_str='red',
        pid=ProcessID('small puck'),
        walls=walls,
        dt = dt)


def mk_big_puck(walls, dt):
    return mk_puck(
        center=Vec2d(SCREEN_WIDTH / 1.5, SCREEN_HEIGHT / 2.5),
        velocity=Vec2d(-1.95, -0.20),
        mass = 100 * 79 * 79 / 42 / 42,
        radius = 79,
        color_str = 'green',
        pid=ProcessID('big puck'),
        walls=walls,
        dt=dt)


def wall_prediction(puck, walls, dt):
    """Dual-use function (classic and time warp). TODO: Move to puck class."""
    predictions = \
        [puck.predict_a_wall_collision(wall, dt) for wall in walls]
    prediction = min(
        predictions,
        key=lambda p: p['tau'] if p['tau'] > 0 else np.inf)
    return prediction


def demo_cage(pause=0.75, dt=1):
    def step_and_draw_both(dt, me, tau, them):
        me.step_many(int(tau), dt)
        them.step_many(int(tau), dt)
        me.draw()
        them.draw()

    def mk_us():
        me = Puck(center=Vec2d(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2),
                  velocity=random_velocity(),
                  mass=100,
                  radius=42,
                  color=THECOLORS['red'])
        them = Puck(center=Vec2d(SCREEN_WIDTH / 1.5, SCREEN_HEIGHT / 2.5),
                    velocity=random_velocity(),
                    mass=100,
                    radius=79,
                    color=THECOLORS['green'])
        return me, them

    def draw_us_with_arrows(me, them):
        me.draw()
        draw_centered_arrow(loc=me.center, vel=me.velocity)
        them.draw()
        draw_centered_arrow(loc=them.center, vel=them.velocity)

    me, them = mk_us()
    clear_screen()
    draw_us_with_arrows(me, them)
    draw_cage()

    # draw_perps_to_cage(me)
    # draw_perps_to_cage(them)

    cage = screen_cage()
    walls = pairwise_toroidal(cage, Wall)

    my_wall_prediction = wall_prediction(me, walls, dt)
    draw_vector(my_wall_prediction['puck_strike_point'],
                my_wall_prediction['wall_strike_point'],
                THECOLORS['purple1'])

    their_wall_prediction = wall_prediction(them, walls, dt)
    draw_vector(their_wall_prediction['puck_strike_point'],
                their_wall_prediction['wall_strike_point'],
                THECOLORS['purple1'])

    my_puck_prediction = me.predict_a_puck_collision(them, dt)

    # pp.pprint({'my_wall_prediction': my_wall_prediction,
    #            'their_wall_prediction': their_wall_prediction,
    #            'my_puck_prediction': my_puck_prediction,
    #            'their_puck_prediction': their_puck_prediction})

    nearest_wall_strike = min([my_wall_prediction,
                               their_wall_prediction],
                              key=lambda p: p['tau'])

    # TODO: can't handle a double wall strike

    if my_puck_prediction['gonna_hit'] and \
            my_puck_prediction['tau'] < nearest_wall_strike['tau']:

        tau = my_puck_prediction['tau']
        step_and_draw_both(dt, me, tau, them)

        n = (them.center - me.center).normalized()

        perp = SCREEN_WIDTH * Vec2d(n[1], -n[0])

        strike = me.center + me.RADIUS * n
        sanity = them.center - them.RADIUS * n

        draw_vector(strike, sanity, THECOLORS['goldenrod1'])

        draw_vector(strike, strike + perp, THECOLORS['limegreen'])
        draw_vector(strike, strike - perp, THECOLORS['maroon1'])

        print({'puck strike': sanity - strike})

    else:  # strike the wall
        tau = nearest_wall_strike['tau']

        step_and_draw_both(dt, me, tau, them)

        print('wall strike')

    assert tau >= 0

    pygame.display.flip()

    time.sleep(pause)


def random_velocity():
    # speed = np.random.randint(1, 5)
    speed = 1 + 4 * np.random.rand()
    # direction = Vec2d(1, 0).rotated(np.random.randint(-2, 2))
    direction = Vec2d(1, 0).rotated(4 * np.random.rand() - 2)
    result = speed * direction
    return result


def demo_hull(pause=7.0):
    clear_screen()
    puck = Puck(
        center=Vec2d(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2),
        velocity=Vec2d(1, 1 / 2),
        mass=100,
        radius=42,
        color=THECOLORS['red'])
    puck.step_many(DEMO_STEPS, DEMO_DT)
    puck.draw()
    hull = convex_hull(random_points(15))
    draw_int_tuples(hull)
    pairwise_toroidal(hull,
                      lambda p0, p1: draw_collinear_point_and_param(
                          Vec2d(p0), Vec2d(p1), line_color=THECOLORS['purple']))
    pygame.display.flip()
    time.sleep(pause)


#   ___ _            _       ___     _ _ _    _
#  / __| |__ _ _____(_)__   / __|___| | (_)__(_)___ _ _  ___
# | (__| / _` (_-<_-< / _| | (__/ _ \ | | (_-< / _ \ ' \(_-<
#  \___|_\__,_/__/__/_\__|  \___\___/_|_|_/__/_\___/_||_/__/


def create_ball(x, y, r, m, color, e=1.0):
    body = pymunk.Body(
        mass=m,
        moment=pymunk.moment_for_circle(
            mass=m,
            inner_radius=0,
            outer_radius=r,
            offset=(0, 0)))
    body.position = x, y
    body.velocity = random_velocity()
    shape = pymunk.Circle(body=body, radius=r)
    shape.color = color
    shape.elasticity = e
    return body, shape


class GameState(object):

    def __init__(self):
        self.space = pymunk.Space()
        self.space.gravity = pymunk.Vec2d(0, 0)

        self.create_walls()
        self.create_obstacle(x=200, y=350, r=DEMO_RADIUS)
        self.create_cat(x=700, y=SCREEN_HEIGHT - PADDING - 100, r=30)
        self.create_car(x=100, y=100, r=25)

    def create_walls(self):
        walls = [
            pymunk.Segment(self.space.static_body, TOP_LEFT, BOTTOM_LEFT, 1),
            pymunk.Segment(self.space.static_body, BOTTOM_LEFT, BOTTOM_RIGHT,
                           1),
            pymunk.Segment(self.space.static_body, BOTTOM_RIGHT, TOP_RIGHT, 1),
            pymunk.Segment(self.space.static_body, TOP_RIGHT, TOP_LEFT, 1),
        ]
        for s in walls:
            s.friction = 1.
            s.elasticity = 0.95
            s.group = 1
            s.collision_type = 1
            s.color = THECOLORS['red']
        self.space.add(walls)

    def add_ball(self, x, y, r, m, c):
        body, shape = create_ball(x, y, r, m, c)
        self.space.add(body, shape)

    def create_obstacle(self, x, y, r, m=100):
        self.add_ball(x, y, r, m, THECOLORS['blue'])

    def create_car(self, x, y, r, m=1):
        self.add_ball(x, y, r, m, THECOLORS['green'])

    def create_cat(self, x, y, r, m=1):
        self.add_ball(x, y, r, m, THECOLORS['orange'])

    def frame_step(self):
        # TODO: no easy way to reset the angle marker after a collision.
        globals.screen.fill(THECOLORS["black"])
        draw(globals.screen, self.space)
        self.space.step(1. / 10)
        pygame.display.flip()


def demo_classic(steps=500):
    game_state = GameState()
    for _ in range(steps):
        game_state.frame_step()


#  __  __      _
# |  \/  |__ _(_)_ _
# | |\/| / _` | | ' \
# |_|  |_\__,_|_|_||_|


def main():
    globals.init_globals()
    set_up_screen()

    PAUSE = 0.75
    STEPS = 1000
    CAGES = 3
    DT = 0.001
    # TODO: There is a bug with multiplicatively increasing times when DT
    # TODO: is small (e.g., 0.001. There is a bug with slowly decreasing times
    # TODO: when DT is 1.

    # demo_classic(steps=STEPS)
    # # input()
    # demo_hull(pause=PAUSE)
    # for _ in range(CAGES):
    #     demo_cage(pause=PAUSE, dt=DT)
    # # input()
    demo_cage_time_warp(drawing=False, pause=0, dt=DT)


if __name__ == "__main__":
    main()
