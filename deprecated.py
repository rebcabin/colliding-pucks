class WallLP(LogicalProcess):
    def __init__(self, wall: Wall, me: ProcessID):
        super().__init__(me)
        self.wall = wall

    def draw(self):
        self.wall.draw()


class PuckLP(LogicalProcess):
    """For Time Warp; contains a puck object.
    TODO: Free logging with the oslash Writer monad.
    TODO: Make this work for more pucks."""

    def _puck_event_2(self, state, msg, walls, dt):
        """TODO: abstract the predictions and movements."""
        result = None
        if msg.body['action'] == 'move':
            result = self._do_move(msg, state, walls, dt)
        elif msg.body['action'] == 'predict':
            result = self._do_prediction(state, walls, dt)
        return result

    def _do_move(self, msg, state, walls, dt):
        # Go where I'm told
        state_prime = self._new_state(Body({
            'center': msg.body['center'],
            'velocity': msg.body['velocity'],
            'walls': walls,
            'dt': dt}))
        self._visualize_puck(state, state_prime, self.now + 1)
        # Schedule a prediction one tiny bit later, a little differently for
        # the two pucks. TODO: this doesn't scale to more pucks. Do table!
        delta_tau = 1 if self.me == 'small puck' else 2
        self.send(rcvr_pid=self.me,
                  receive_time=self.now + delta_tau,
                  body=Body({'action': 'predict'}))
        return state_prime

    def _do_prediction(self, state, walls, dt):
        state_prime = state  # don't move
        wall_pred = wall_prediction(self.puck, walls, dt)
        tau_wall = wall_pred['tau']
        future_other_puck, other_lp = self._get_other(dt)  # TODO: inaccurate?
        puck_pred = self.puck.predict_a_puck_collision(
            future_other_puck, dt)
        tau_puck = puck_pred['tau']
        if puck_pred['gonna_hit'] and 0 < tau_puck < tau_wall:
            self._schedule_puck_puck_collision(other_lp, puck_pred, tau_puck)
        else:
            self._schedule_wall_puck_collision(tau_wall, wall_pred)
        return state_prime

    def _schedule_puck_puck_collision(self, other_lp, puck_pred, tau_puck):
        self._report_puck_prediction(other_lp, tau_puck)
        self._check_conservation_for_puck_prediction(puck_pred)
        then = self.now + tau_puck
        self.send(rcvr_pid=self.me,
                  receive_time=then,
                  body=Body({'action': 'move',
                             'center': puck_pred["c1'"],
                             'velocity': puck_pred["v1'"]}))
        self.send(rcvr_pid=other_lp.me,
                  receive_time=then,
                  body=Body({'action': 'move',
                             'center': puck_pred["c2'"],
                             'velocity': puck_pred["v2'"]}))

    def _schedule_wall_puck_collision(self, tau_wall, wall_pred):
        self._report_wall_prediction(tau_wall)
        self._check_conservation_for_wall_prediction(wall_pred)
        self.send(rcvr_pid=self.me,
                  receive_time=self.now + tau_wall,
                  body=Body({'action': 'move',
                             'center': wall_pred["c'"],
                             'velocity': wall_pred["v'"]}))

    @staticmethod
    def _check_conservation_for_wall_prediction(wall_pred):
        # Wall collision does not preserve puck momentum.
        # assert wall_pred["delta p"] < 1e-12
        # print({'wall dt': wall_pred['tau']})
        assert np.abs(wall_pred["delta k"]) < 1e-10

    @staticmethod
    def _check_conservation_for_puck_prediction(puck_pred):
        assert np.abs(puck_pred["delta k"]) < 1e-10
        assert puck_pred["delta p"] < 1e-10

    def _report_wall_prediction(self, tau_wall):
        # pp.pprint({'coll_pred': self.me,
        #            'with': 'wall',  # TODO: get the 'me' names
        #            'tau': tau_wall,
        #            'pred_lvt': self.now + tau_wall or 1,
        #            'now': self.now})
        pass

    def _report_puck_prediction(self, its_lp, tau_puck):
        # print({'coll_pred': self.me,
        #        'with': its_lp.me,
        #        'tau': tau_puck,
        #        'pred_lvt': self.now + tau_puck,
        #        'now': self.now})
        pass

    def event_main(self, lvt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        assert lvt == self.vt
        assert lvt == self.now

        # Move the puck physics state to the commanded tw-state:
        self.puck.center = state.body['center']
        self.puck.velocity = state.body['velocity']

        walls = state.body['walls']
        dt = state.body['dt']
        # Give priority to 'predict' messages. If there are two move commands
        # at the same time, pick the one from the other puck. All this is a
        # likely consequence of the fact that we don't have accurate
        # predictions in 'get_other' TODO: unverified.
        if len(msgs) != 1:
            msg = msgs[0]  # Pick one arbitrarily. If there are two moves
            # from self to self, they are likely nearly identical states
            # TODO: we checked one case by hand in the debugger.
            for m in msgs:
                if m.body['action'] == 'predict':
                    msg = m
                    break
                elif m.sender != m.receiver:
                    assert m.body['action'] == 'move'
                    msg = m
                    break
            assert msg is not None
        else:
            assert len(msgs) == 1
            msg = msgs[0]

        result = self._puck_event_2(state, msg, walls, dt)
        return result

    def _get_other(self, dt):
        other_pid = 'big puck' if self.me == 'small puck' \
            else 'small puck'
        future_puck, its_tw_state = self.query(
            other_pid,
            Body({'dt': dt}))
        its_lp = globals.world_map[other_pid]
        return future_puck, its_lp

    def _visualize_puck(self, state, state_prime, then):
        """Temporary method for debugging collisions. Aso of Tue, 24 July 2018,
        I'm convinced the collision geometry is correct."""
        c = state.body['center']
        v = state.body['velocity'] * 100
        c_prime = state_prime.body['center']
        v_prime = state_prime.body['velocity'] * 100
        int_center = state_prime.body['center'].int_tuple
        pygame.draw.circle(
            globals.screen,
            self.puck.COLOR,
            int_center,
            self.puck.RADIUS,
            self.puck.DONT_FILL_BIT
        )
        pygame.draw.circle(
            globals.screen,
            THECOLORS['black'],
            int_center,
            self.puck.RADIUS,
            True  # don't fill
        )
        text = str(then)
        rtext = myfont.render(text, True, THECOLORS['white'], THECOLORS['black'])
        draw_vector(c_prime, c_prime + v, THECOLORS['gray60'])
        draw_vector(c_prime, c_prime + v_prime, THECOLORS['magenta'])
        globals.screen.blit(rtext, int_center)
        pygame.display.flip()
        time.sleep(0)

    def query_main(self,
                   vt: VirtualTime,
                   state: State,
                   msg: QueryMessage) -> Tuple['Puck', State]:
        """Someone is asking where I will be at time vt. Do a straight
        prediction without checking for collisions or decay of motion."""
        # TODO: This could be seriously wrong if it misses collisions.
        # TODO: Return a query-response message.
        assert vt >= state.vt  # units of ticks
        dt = msg.body['dt']
        # Special case for boot-time state
        new_center = \
            self.puck.center + dt * (vt - state.vt) * self.puck.velocity \
            if state.vt > (- sys.maxsize) else self.puck.center
        return Puck(
            center=new_center,
            velocity=self.puck.velocity,
            mass=self.puck.MASS,
            radius=self.puck.RADIUS,
            color=THECOLORS['white'],
            dont_fill_bit=self.puck.DONT_FILL_BIT
        ), state

    def __init__(self, puck: Puck, me: ProcessID):
        super().__init__(me)
        self.puck = puck

    def draw(self):
        self.puck.draw()


def demo_cage_time_warp_1(drawing=True, pause=0.75, dt=1):
    """"""
    clear_screen()
    # TODO: Drawing should happen as side effect of first event messages

    wall_lps = mk_walls_lps()
    [w.draw() for w in wall_lps]
    walls = [w.wall for w in wall_lps]

    small_puck_lp = mk_small_puck_lp(walls, dt)
    small_puck_lp.draw()
    draw_centered_arrow(small_puck_lp.puck.center,
                        small_puck_lp.puck.velocity)

    big_puck_lp = mk_big_puck_lp(walls, dt)
    big_puck_lp.draw()
    draw_centered_arrow(big_puck_lp.puck.center,
                        big_puck_lp.puck.velocity)

    # boot the OS
    for wall_lp in wall_lps:
        globals.sched_q.insert(wall_lp)
    globals.sched_q.insert(small_puck_lp)
    globals.sched_q.insert(big_puck_lp)

    # boot the simulation:
    small_puck_lp.send(
        rcvr_pid=ProcessID('small puck'),
        receive_time=VirtualTime(0),
        body=Body({
            'action': 'predict'
        }),
        force_send_time=EARLIEST_VT)
    print({'sending': 'BOOT move',
           'receiver': 'small puck',
           'send time': EARLIEST_VT,
           'receive_time': 0})

    globals.sched_q.run(drawing=drawing, pause=pause)

    pygame.display.flip()
    time.sleep(pause)


def mk_walls_lps():
    wall_lps = [WallLP(Wall(SCREEN_TL, SCREEN_BL), "left wall"),
                WallLP(Wall(SCREEN_BL, SCREEN_BR), "bottom wall"),
                WallLP(Wall(SCREEN_BR, SCREEN_TR), "right wall"),
                WallLP(Wall(SCREEN_TR, SCREEN_TL), "top wall")]
    return wall_lps


def mk_puck_lp(center, velocity, mass, radius, color_str, pid, walls, dt):
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


def mk_small_puck_lp(walls, dt):
    return mk_puck_lp(
        center=Vec2d(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2),
        velocity=Vec2d(2.3, -1.7),
        mass=100,
        radius=42,
        color_str='red',
        pid=ProcessID('small puck'),
        walls=walls,
        dt = dt)


def mk_big_puck_lp(walls, dt):
    return mk_puck_lp(
        center=Vec2d(SCREEN_WIDTH / 1.5, SCREEN_HEIGHT / 2.5),
        velocity=Vec2d(-1.95, -0.20),
        mass = 100 * 79 * 79 / 42 / 42,
        radius = 79,
        color_str = 'green',
        pid=ProcessID('big puck'),
        walls=walls,
        dt=dt)


