import itertools
import copy

import globals

from pygame.color import THECOLORS

import pymunk
from pymunk.pygame_util import draw

from tw_types import *
from rendering import *
from funcyard import *

# TODO: Convert the tw_types from dicts to namedtuples.
from collections import namedtuple
from locutius.multimethods import multi, multimethod, method

import pprint
pp = pprint.PrettyPrinter(indent=2)

pygame.font.init()
myfont = pygame.font.SysFont('Courier', 30)


# TODO: IDs might be best as uuids.
# TODO: Message equality can be optimized.
# TODO: Off-by-one rendering error on bottom and right.


#  _____     _    _       ___          _
# |_   _|_ _| |__| |___  | _ \___ __ _(_)___ _ _
#   | |/ _` | '_ \ / -_) |   / -_) _` | / _ \ ' \
#   |_|\__,_|_.__/_\___| |_|_\___\__, |_\___/_||_|
#                                |___/


TableStateBody = namedtuple('TableState', ['pucks', 'walls', 'dt'])
TableEventMessageBody = namedtuple('TableEventMessage', ['action', 'contents'])
PuckPrediction = namedtuple('PuckPrediction', ['region',
                                               'p1', 'p2',
                                               'c1', 'c2',
                                               'v1', 'v2',
                                               'steps',
                                               'walls', 'pucks', 'dt'])
WallPrediction = namedtuple('WallPrediction', ['region', 'puck', 'c', 'v',
                                               'steps',
                                               'walls', 'pucks', 'dt'])


@method(PuckPrediction)
def new_state(prediction):
    prediction.p1.center = prediction.c1
    prediction.p2.center = prediction.c2
    prediction.p1.velocity = prediction.v1
    prediction.p2.velocity = prediction.v2
    result = prediction.region._new_state(
        body=TableStateBody(
            walls=prediction.walls,
            pucks=prediction.pucks,
            dt=prediction.dt))
    return result


@method(WallPrediction)
def new_state(prediction):
    prediction.puck.center = prediction.c
    prediction.puck.velocity = prediction.v
    for p in prediction.pucks:
        if p is not prediction.puck:
            p.center += p.velocity * prediction.dt * prediction.steps
    result = prediction.region._new_state(
        body=TableStateBody(
            walls=prediction.walls,
            pucks=prediction.pucks,
            dt=prediction.dt))
    return result


class TableRegion(LogicalProcess):

    def __init__(self, me: ProcessID):
        super().__init__(me)

    @staticmethod
    def mk_walls():
        wall_lps = [ Wall(SCREEN_TL, SCREEN_BL),
                     Wall(SCREEN_BL, SCREEN_BR),
                     Wall(SCREEN_BR, SCREEN_TR),
                     Wall(SCREEN_TR, SCREEN_TL), ]
        return wall_lps

    @staticmethod
    def mk_pucks():
        small_puck = Puck(
            center=Vec2d(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2),
            velocity=Vec2d(2.3, -1.7),
            mass=100,
            radius=42,
            color=THECOLORS['red'], )

        big_puck = Puck(
            center=Vec2d(SCREEN_WIDTH / 1.5, SCREEN_HEIGHT / 2.5),
            velocity=Vec2d(-1.95, -0.20),
            mass=100 * 79 * 79 / 42 / 42,
            radius=79,
            color=THECOLORS['green'], )

        return [small_puck, big_puck]

    def event_main(self, lvt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        assert lvt == self.vt
        assert lvt == self.now

        walls = state.body.walls
        pucks = state.body.pucks
        dt = state.body.dt

        # TODO: avoid LP 'draw' method because it relies on setting up a shadow
        # TODO: state in self.<instance_variables>, and we don't need that.
        # TODO: Eventually remove LogicalProcess.draw.
        result = None
        for msg in msgs:
            if msg.body.action == 'draw':
                self._draw(walls, pucks)
                result = self._new_state(body=state.body)
            elif msg.body.action == 'predict':
                self._predict(walls, pucks, dt)
                result = self._new_state(body=state.body)
            elif msg.body.action == 'move':
                clear_screen()
                # self._draw(walls, pucks)
                self._animate(walls, pucks, msg.body.contents.steps, dt)
                result = new_state(msg.body.contents)
                self._draw(walls, pucks)
                self._predict(walls, pucks, dt)
        return result

    def _predict(self, walls, pucks, dt):
        def realistic_time(p):
            return p['tau'] if p['tau'] > 0 else sys.maxsize

        wall_preds = [puck.predict_a_wall_collision(wall, dt)
                      for puck in pucks
                      for wall in walls]
        earliest_wall_pred = min(wall_preds, key=realistic_time)

        puck_preds_pre = [p1.predict_a_puck_collision(p2, dt)
                          for p1, p2 in itertools.combinations(pucks, 2)]
        puck_preds = [p for p in puck_preds_pre
                      if p['gonna_hit'] and p['tau'] > 0]
        earliest_puck_pred = \
            min(puck_preds, key=realistic_time) \
            if puck_preds else None

        assert earliest_wall_pred or earliest_puck_pred

        if earliest_puck_pred is None \
                or earliest_wall_pred['tau'] <= earliest_puck_pred['tau']:
            self._schedule_update(
                self.now + earliest_wall_pred['tau'],
                WallPrediction(
                    region=self,
                    steps=earliest_wall_pred['tau'],
                    puck=earliest_wall_pred['puck_victim'],
                    c=earliest_wall_pred["c'"],
                    v=earliest_wall_pred["v'"],
                    walls=walls,  # TODO: State monad
                    pucks=pucks,
                    dt=dt))
        else:
            self._schedule_update(
                self.now + earliest_puck_pred['tau'],
                PuckPrediction(
                    region=self,
                    steps=earliest_puck_pred['tau'],
                    p1=earliest_puck_pred['puck_self'],
                    p2=earliest_puck_pred['puck_victim'],
                    c1=earliest_puck_pred["c1'"],
                    v1=earliest_puck_pred["v1'"],
                    c2=earliest_puck_pred["c2'"],
                    v2=earliest_puck_pred["v2'"],
                    walls=walls,  # TODO: State monad
                    pucks=pucks,
                    dt=dt
                ))

    def _schedule_update(self, then, prediction):
        self.send(
            rcvr_pid=self.me,
            receive_time=then,
            body=TableEventMessageBody(
                action='move',
                contents=prediction))

    @staticmethod
    def _animate(walls, pucks, steps, dt):
        ps = copy.deepcopy(pucks)
        JUMP = 100
        for _ in range(0, steps, JUMP):
            clear_screen()
            TableRegion._draw(walls, ps)
            pygame.display.flip()
            for p in ps:
                p.center += p.velocity * JUMP * dt

    @staticmethod
    def _draw(walls, pucks):
        """TODO: This is not the 'draw' method of the logical-process
        TODO: superclass. We're going to get rid of that."""
        for wall in walls:
            wall.draw()
        for puck in pucks:
            puck.draw()
        pygame.display.flip()


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
        pygame.draw.circle(
            globals.screen,
            self.COLOR,
            self.center.int_tuple,
            self.RADIUS,
            self.DONT_FILL_BIT)
        draw_vector(
            self.center,
            self.center + 100 * self.velocity,
            THECOLORS['magenta'])

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
        return {  # TODO: make named tuple
            'collision_type': 'wall',
            'tau': int(tau) if tau < np.inf else sys.maxsize,
            'tau_physical': tau_physical,
            "delta k": k - k_prime,
            "delta p": (p - p_prime).length,
            'puck_strike_point': p_c,
            'wall_strike_point': q_prime,
            'wall_strike_parameter': t_prime,
            'puck_victim': self,
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
        # notation from high-school algebra.
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
                gonna_hit = True
            else:
                tau_physical = float(max(root1, root2))
                gonna_hit = root1 > 0 or root2 > 0
            tau = tau_physical / dt
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
            k_prime, p_prime = self.puck_puck_energy_momentum(
                them, v1_prime, v2_prime)
        result = {  # TODO: make named tuple
            'collision_type': 'puck',
            'tau': int(tau) if tau < np.inf else sys.maxsize,
            'tau_physical': tau_physical,
            "delta k": k - k_prime if k_prime else 0,
            "delta p": (p - p_prime).length if p_prime else 0,
            'puck_self': self,
            'puck_victim': them,
            'gonna_hit': gonna_hit,
            "c1'": c1_prime,  # TODO: can't index later with string "c1'"
            "v1'": v1_prime,
            "c2'": c2_prime,
            "v2'": v2_prime}
        return result

    def puck_puck_energy_momentum(self, them, v1, v2):
        # momentum
        p1 = self.MASS * v1
        p2 = them.MASS * v2
        p = p1 + p2
        # energy
        k1 = p1.dot(p1) / 2 / self.MASS
        k2 = p2.dot(p2) / 2 / them.MASS
        k = k1 + k2
        return k, p


#  _______      __   ___                 ___
# |_   _\ \    / /  / __|__ _ __ _ ___  |   \ ___ _ __  ___
#   | |  \ \/\/ /  | (__/ _` / _` / -_) | |) / -_) '  \/ _ \
#   |_|   \_/\_/    \___\__,_\__, \___| |___/\___|_|_|_\___/
#                            |___/


def demo_cage_time_warp(drawing=True, event_pause=0.75,
                        final_pause=3.00, dt=0.001):
    table_0_0_id = ProcessID('table region 0 0')
    table_region_lp = TableRegion(me=table_0_0_id)
    initial_table_state = State(
        sender=table_0_0_id,
        send_time=EARLIEST_VT,
        body=TableStateBody(
            pucks=TableRegion.mk_pucks(),
            walls=TableRegion.mk_walls(),
            dt=dt))
    table_region_lp.sq.insert(initial_table_state)
    globals.sched_q.insert(table_region_lp)
    table_region_lp.send(
        rcvr_pid=table_0_0_id,
        receive_time=VirtualTime(0),
        force_send_time=EARLIEST_VT,
        body=TableEventMessageBody(
            action='draw',
            contents=None
        ))
    table_region_lp.send(
        rcvr_pid=table_0_0_id,
        receive_time=VirtualTime(0),
        force_send_time=EARLIEST_VT,
        body=TableEventMessageBody(
            action='predict',
            contents=None
        ))

    clear_screen()
    globals.sched_q.run(drawing=drawing, pause=event_pause)
    pygame.display.flip()

    time.sleep(final_pause)


#  ___            _     _   _           ___             _   _
# |   \ _  _ __ _| |___| | | |___ ___  | __|  _ _ _  __| |_(_)___ _ _  ___
# | |) | || / _` | |___| |_| (_-</ -_) | _| || | ' \/ _|  _| / _ \ ' \(_-<
# |___/ \_,_\__,_|_|    \___//__/\___| |_| \_,_|_||_\__|\__|_\___/_||_/__/


def wall_prediction(puck, walls, dt):
    """Dual-use function (classic and time warp). TODO: Move to puck class."""
    predictions = \
        [puck.predict_a_wall_collision(wall, dt) for wall in walls]
    prediction = min(
        predictions,
        key=lambda p: p['tau'] if p['tau'] > 0 else np.inf)
    return prediction


#  _  _            _______      __  ___
# | \| |___ _ _ __|_   _\ \    / / |   \ ___ _ __  ___ ___
# | .` / _ \ ' \___|| |  \ \/\/ /  | |) / -_) '  \/ _ (_-<
# |_|\_\___/_||_|   |_|   \_/\_/   |___/\___|_|_|_\___/__/


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
            pymunk.Segment(self.space.static_body, BOTTOM_LEFT, BOTTOM_RIGHT, 1),
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

    # demo_classic(steps=STEPS)
    # # input()
    # demo_hull(pause=PAUSE)
    # for _ in range(CAGES):
    #     demo_cage(pause=PAUSE, dt=DT)
    # # input()
    demo_cage_time_warp(drawing=False, event_pause=0.75, dt=DT)


if __name__ == "__main__":
    main()
