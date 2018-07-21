import sys

import pygame
from pygame.color import THECOLORS

import pymunk
from pymunk.vec2d import Vec2d
from pymunk.pygame_util import draw

import time

import numpy as np
import numpy.random as rndm

from typing import List, Tuple, Callable, Dict, Any

import sortedcontainers

import pprint
pp = pprint.PrettyPrinter(indent=2)


#   ___             _            _
#  / __|___ _ _  __| |_ __ _ _ _| |_ ___
# | (__/ _ \ ' \(_-<  _/ _` | ' \  _(_-<
#  \___\___/_||_/__/\__\__,_|_||_\__/__/


SCREEN_WIDTH = 1000
SCREEN_HEIGHT = 700
DEMO_STEPS = 100
DEMO_DT = 1.00
DEMO_RADIUS = 100
PADDING = 10

SPOT_RADIUS = 9

TOP_LEFT = (PADDING, PADDING)
BOTTOM_LEFT = (PADDING, SCREEN_HEIGHT - PADDING - 1)
BOTTOM_RIGHT = (SCREEN_WIDTH - PADDING - 1, SCREEN_HEIGHT - PADDING - 1)
TOP_RIGHT = Vec2d(SCREEN_WIDTH - PADDING - 1, PADDING)

# Counterclockwise for a correctly oriented polygon according to the Jordan
# curve convention.
SCREEN_TL = Vec2d(TOP_LEFT)
SCREEN_BL = Vec2d(BOTTOM_LEFT)
SCREEN_BR = Vec2d(BOTTOM_RIGHT)
SCREEN_TR = Vec2d(TOP_RIGHT)

EARLIEST_VT = -sys.maxsize
LATEST_VT = sys.maxsize


# TODO: IDs might be best as uuids.
# TODO: Message equality can be optimized.


# __   ___     _             _ _____ _
# \ \ / (_)_ _| |_ _  _ __ _| |_   _(_)_ __  ___
#  \ V /| | '_|  _| || / _` | | | | | | '  \/ -_)
#   \_/ |_|_|  \__|\_,_\__,_|_| |_| |_|_|_|_\___|


class VirtualTime(int):
    """Establishes a type for Virtual Time; it's an int."""


#  ___                         ___ ___
# | _ \_ _ ___  __ ___ ______ |_ _|   \
# |  _/ '_/ _ \/ _/ -_|_-<_-<  | || |) |
# |_| |_| \___/\__\___/__/__/ |___|___/


class ProcessID(str):
    """Establishes a type for Process ID; it's a string."""


#  __  __                            ___          _
# |  \/  |___ ______ __ _ __ _ ___  | _ ) ___  __| |_  _
# | |\/| / -_|_-<_-</ _` / _` / -_) | _ \/ _ \/ _` | || |
# |_|  |_\___/__/__/\__,_\__, \___| |___/\___/\__,_|\_, |
#                        |___/                      |__/


class Body(Dict):
    """Establishes a type for message bodies; they're Dicts."""


#  _____ _              _                           _
# |_   _(_)_ __  ___ __| |_ __ _ _ __  _ __  ___ __| |
#   | | | | '  \/ -_|_-<  _/ _` | '  \| '_ \/ -_) _` |
#   |_| |_|_|_|_\___/__/\__\__,_|_|_|_| .__/\___\__,_|
#                                     |_|


# See https://goo.gl/hiwMgJ


class ComparableMixin(object):
    def _compare(self, other, method):
        try:
            return method(self._cmpkey(), other._cmpkey())
        except (AttributeError, TypeError):
            # _cmpkey not implemented, or return different type,
            # so I can't compare with "other".
            return NotImplemented

    def __lt__(self, other):
        return self._compare(other, lambda s, o: s < o)

    def __le__(self, other):
        return self._compare(other, lambda s, o: s <= o)

    def __eq__(self, other):
        return self._compare(other, lambda s, o: s == o)

    def __ge__(self, other):
        return self._compare(other, lambda s, o: s >= o)

    def __gt__(self, other):
        return self._compare(other, lambda s, o: s > o)

    def __ne__(self, other):
        return self._compare(other, lambda s, o: s != o)


class Timestamped(ComparableMixin):
    """Establishes a type for timestamped objects:
    messages, states, logical processes."""

    def __init__(self, vt: VirtualTime):
        self.vt = vt

    def _cmpkey(self):
        return self.vt


#  ___             _     __  __
# | __|_ _____ _ _| |_  |  \/  |___ ______ __ _ __ _ ___
# | _|\ V / -_) ' \  _| | |\/| / -_|_-<_-</ _` / _` / -_)
# |___|\_/\___|_||_\__| |_|  |_\___/__/__/\__,_\__, \___|
#                                              |___/


class EventMessage(Timestamped):
    """The vt field is new over classical time warp. It is a convenience to make
    the common case easy. Positive messages have vt=receive_time by default;
    negative have vt=send_time. That makes easy the common case of inserting
    positive messages into input queues and negative messages into output
    queues. When a negative message is inserted into an input queue, its vt
    must be switched to receive time. Likewise, when a positive message is
    inserted into an output queue, its vt must be switched to send time."""

    def __init__(self,
                 sender: ProcessID,
                 send_time: VirtualTime,
                 receiver: ProcessID,
                 receive_time: VirtualTime,
                 sign: bool,
                 body: Body):
        if receive_time <= send_time:
            raise ValueError(f"receive time {receive_time} must be strictly "
                             f"greater than send time {send_time}")
        super().__init__(vt=receive_time if sign else send_time)
        self.sender = sender
        self.send_time = send_time
        self.receiver = receiver
        self.receive_time = receive_time
        self.sign = sign
        self.body = body

    def __eq__(self, other: 'EventMessage'):
        """Check equality for all attributes EXCEPT algebraic sign.
        TODO: optimize with hashes or uuids."""
        return self.sender == other.sender and \
               self.send_time == other.send_time and \
               self.receiver == other.receiver and \
               self.receive_time == other.receive_time and \
               self.body == other.body

    def __ne__(self, other: 'EventMessage'):
        """Don't use default timestamp comparable for !=."""
        return not self == other


#  _______      _____ _        _
# |_   _\ \    / / __| |_ __ _| |_ ___
#   | |  \ \/\/ /\__ \  _/ _` |  _/ -_)
#   |_|   \_/\_/ |___/\__\__,_|\__\___|


class State(EventMessage):
    def __init__(self,
                 sender: ProcessID,
                 send_time: VirtualTime,
                 body: Body):
        """Modeled as a negative event message from self to self with
        indeterminate (positive infinite) receive time. Its message sign
        negative, so its timestamp is the send time, just like an output.
        negative message."""
        super().__init__(sender=sender,
                         send_time=send_time,
                         receiver=sender,
                         receive_time=LATEST_VT,
                         sign=False,
                         body=body)


#   ___                      __  __
#  / _ \ _  _ ___ _ _ _  _  |  \/  |___ ______ __ _ __ _ ___
# | (_) | || / -_) '_| || | | |\/| / -_|_-<_-</ _` / _` / -_)
#  \__\_\\_,_\___|_|  \_, | |_|  |_\___/__/__/\__,_\__, \___|
#                     |__/                         |___/


class QueryMessage:
    """TODO"""


#  _______      _____
# |_   _\ \    / / _ \ _  _ ___ _  _ ___
#   | |  \ \/\/ / (_) | || / -_) || / -_)
#   |_|   \_/\_/ \__\_\\_,_\___|\_,_\___|


class TWQueue(object):
    """Implements timestamp-ordered, vt-cursored queue with annihilation.
    Internally, a sorted dictionary of lists, each list being a 'bundle'
    of elements all with the same virtual time. The internal dictionary is
    called 'elements' to prevent confusion with the Python dictionary primitive
    'items(),' which produces a sequence of tuples."""

    def __init__(self):
        # See http://www.grantjenks.com/docs/sortedcontainers/
        # Efficient search, insertion, next.
        # Keys are virtual times. Values are lists of Timestamped's.
        self.elements = sortedcontainers.SortedDict()
        self.vt = LATEST_VT
        self.rollback = False
        self.annihilation = False

    def vts(self):
        """For debugging"""
        return [e[0] for e in self.elements.items()]

    def latest_earlier_time(self, vt: VirtualTime):
        """Produce the latest key in the dictionary earlier than the given
        virtual time."""
        if self.elements == {}:
            return EARLIEST_VT
        # produce index of latest element equal to or greater than vt:
        l = self.elements.bisect_left(vt)
        if l == 0:
            return EARLIEST_VT
        else:
            # peekitem returns a key-value tuple; we want just the key:
            return self.elements.peekitem(l - 1)[0]

    def earliest_later_time(self, vt: VirtualTime):
        """Produce the earliest key in the dictionary later than the given
        virtual time."""
        if self.elements == {}:
            return LATEST_VT
        # produce index of latest element equal to or greater than vt:
        l = self.elements.bisect_left(vt)
        length = len(self.elements)
        if l == length:
            return LATEST_VT
        if self.elements.peekitem(l)[0] == vt:
            if l < length - 1:
                return self.elements.peekitem(l + 1)[0]
            else:
                return LATEST_VT
        else:
            return self.elements.peekitem(l)[0]

    def insert(self, m: Timestamped):
        self.annihilation = False
        if m.vt <= self.vt:
            self.rollback = True
            # Even if there is eventual annihilation, we need to roll
            # back to this time or earlier:
            self.vt = m.vt
        if m.vt in self.elements:
            for e in self.elements[m.vt]:
                if (e == m and
                        hasattr(m, 'sign') and
                        hasattr(e, 'sign') and
                        e.sign == (not m.sign)):
                    self.annihilation = True
                    self.elements[m.vt].remove(e)
                    # If there are no more timestamped's, kill the key and
                    # move the rollback time
                    if self.elements[m.vt] == []:
                        self.elements.pop(m.vt)
                        if self.rollback:
                            self.vt = self.latest_earlier_time(m.vt)
            if not self.annihilation:
                self.elements[m.vt].append(m)
        else:
            self.elements[m.vt] = [m]

    def insert_bundle(self, ms: List[Timestamped]):
        """Optimization when it is known there will be no annihlation"""
        if ms:
            key = ms[0].vt
            if key in self.elements:
                raise ValueError(f'cannot insert a bundle into a non-empty '
                                 f'slot of virtual time.')
            self.elements[ms[0].vt] = ms

    def remove(self, vt: VirtualTime):
        """Let this raise the natural KeyError if the vt is not in the queue."""
        return self.elements.pop(vt)

    def remove_earliest(self):
        return self.elements.popitem(0)


#  ___ _        _          ___
# / __| |_ __ _| |_ ___   / _ \ _  _ ___ _  _ ___
# \__ \  _/ _` |  _/ -_) | (_) | || / -_) || / -_)
# |___/\__\__,_|\__\___|  \__\_\\_,_\___|\_,_\___|


class StateQueue(TWQueue):
    def __init__(self):
        super().__init__()


#   ___       _             _      ___
#  / _ \ _  _| |_ _ __ _  _| |_   / _ \ _  _ ___ _  _ ___
# | (_) | || |  _| '_ \ || |  _| | (_) | || / -_) || / -_)
#  \___/ \_,_|\__| .__/\_,_|\__|  \__\_\\_,_\___|\_,_\___|
#                |_|


class OutputQueue(TWQueue):
    def __init__(self):
        super().__init__()

    def insert(self, message: EventMessage):
        if message.sign:
            message.vt = message.send_time
        super().insert(message)


#  ___                _      ___
# |_ _|_ _  _ __ _  _| |_   / _ \ _  _ ___ _  _ ___
#  | || ' \| '_ \ || |  _| | (_) | || / -_) || / -_)
# |___|_||_| .__/\_,_|\__|  \__\_\\_,_\___|\_,_\___|
#          |_|


class InputQueue(TWQueue):
    def __init__(self):
        super().__init__()
        self.vt = LATEST_VT

    def insert(self, message: EventMessage):
        if not message.sign:
            message.vt = message.receive_time
        super().insert(message)


#  ___     _           _      _        ___
# / __| __| |_  ___ __| |_  _| |___   / _ \ _  _ ___ _  _ ___
# \__ \/ _| ' \/ -_) _` | || | / -_) | (_) | || / -_) || / -_)
# |___/\__|_||_\___\__,_|\_,_|_\___|  \__\_\\_,_\___|\_,_\___|


class ScheduleQueue(TWQueue):
    def __init__(self):
        super().__init__()
        self.world_map = {}

    def insert(self, lp: 'LogicalProcess'):
        self.world_map[lp.me] = lp
        super().insert(lp)

    def run(self):
        """Pop he first (earliest) lp in the queue (if there is one). Run it
        until it terminates or until it's interrupted by arrival of a new
        message to the processor. If the new message causes some other LP to
        become earliest, suspend this one for later resumption.

        In a real OS, we would suspend the process's machine state and restore
        that later in real time when the process becomes earliest in virtual
        time again. Here, where we don't have easy access to machine state of a
        process, we'll just restart it."""

        # Local Virtual Time is called 'lvt' in most time-warp papers. This
        # should not be a mysterious or confusing acronym.
        earliest_item = self.elements.peekitem(0)
        lp = earliest_item[1]
        lvt = earliest_item[0]

        input_bundle = lp.iq[lvt]  # Let iq throw if no input messages!
        state = lp.sq.get(lvt, {})
        state_prime = lp.event_main(lvt, state, input_bundle)

        pass


#  _              _         _   ___
# | |   ___  __ _(_)__ __ _| | | _ \_ _ ___  __ ___ ______
# | |__/ _ \/ _` | / _/ _` | | |  _/ '_/ _ \/ _/ -_|_-<_-<
# |____\___/\__, |_\__\__,_|_| |_| |_| \___/\__\___/__/__/
#           |___/


class LogicalProcess(Timestamped):
    def __init__(self,
                 me: ProcessID,
                 event_main: Callable[[VirtualTime, State, List[EventMessage]],
                                      State],
                 query_main: Callable[[VirtualTime, State, List[QueryMessage]],
                                      State] = None):
        self.now = LATEST_VT
        super().__init__(self.now)
        self.iq = InputQueue()
        self.oq = OutputQueue()
        self.sq = StateQueue()
        self.me = me
        self.event = event_main
        self.query = query_main

    def new_state(self, body: Body):
        return State(sender=self.me,
                     send_time=self.now,
                     body=body)

    def send(self,
             other: ProcessID,
             receive_time: VirtualTime,
             body: Body,
             force_send_time = None):  # for boot only
        # TODO: The rest of this stuff belongs in the Schedule Queue.
        global sched_q
        other_lp = sched_q.world_map[other]
        msg, antimsg = self._message_pair(
            other, force_send_time, receive_time, body)
        self.oq.insert(antimsg)
        if self.oq.annihilation:
            raise ValueError(
                f'unexpected annihilation from antimessage '
                f'{pp.pformat(antimsg)} at process '
                f'{pp.pformat(self)}')
        other_lp.iq.insert(msg)
        if other_lp.iq.rollback:
            new_lp_vt = other_lp.iq.vt
            lp_bundle = sched_q.elements.pop(self.now)
            # find self in the bundle
            for i in range(len(lp_bundle)):
                if self is lp_bundle[i]:
                    pre = lp_bundle[:i]
                    post = lp_bundle[i + 1:]
                    me = lp_bundle[i]
                    residual = pre + post
                    sched_q.insert_bundle(residual)
                    me.now = me.vt = new_lp_vt
                    sched_q.insert(me)
                    break

    def _message_pair(self, other, force_send_time, receive_time, body):
        send_time = force_send_time or self.now
        message = EventMessage(
            sender=self.me,
            send_time=send_time,
            receiver=other,
            receive_time=receive_time,
            sign=True,
            body=body
        )
        antimessage = EventMessage(
            sender=self.me,
            send_time=send_time,
            receiver=other,
            receive_time=receive_time,
            sign=False,
            body=body
        )
        return message, antimessage

    def query(self, other: ProcessID, body: Body):
        pass


class WallLP(LogicalProcess):

    def __init__(self, wall, me: ProcessID, event_main, query_main):
        super().__init__(me, event_main, query_main)
        self.wall = wall


class PuckLP(LogicalProcess):

    def event_main(self, vt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        self.vt = vt
        self.puck.center = state.body['center']
        self.puck.velocity = state.body['velocity']
        # In general, the puck may move to table sectors with
        # different walls, so the list of walls must be in the
        # tw-state. Likewise, some collision schemes may vary
        # dt, so we have it in the state.
        walls = state.body['walls']
        dt = state.body['dt']
        for msg in msgs:
            if msg.body['action'] == 'move':
                pred = wall_prediction(self.puck, walls, dt)
                state_prime = \
                    self.new_state(
                        Body({
                            'center': self.puck.center \
                                      + pred['tau'] * dt * self.puck.velocity,
                            # elastic, frictionless collision
                            'velocity': \
                                + pred['v_t'] * pred['t'] \
                                - pred['v_n'] * pred['n'],
                            'walls': walls,
                            'dt': dt
                        }))
                self.send(other=self.me,
                          receive_time=self.me + pred['tau'],
                          body=Body({'action': 'move'}))
                return state_prime
            else:
                raise ValueError(f'unknown message body '
                                 f'{pp.pformat(msg)} '
                                 f'for puck {pp.pformat(self.puck)}')

    def __init__(self, puck, me: ProcessID, query_main):
        super().__init__(me, self.event_main, query_main)
        self.puck = puck


#  ___ _           _         _   ___
# | _ \ |_ _  _ __(_)__ __ _| | | _ \_ _ ___  __ ___ ______ ___ _ _
# |  _/ ' \ || (_-< / _/ _` | | |  _/ '_/ _ \/ _/ -_|_-<_-</ _ \ '_|
# |_| |_||_\_, /__/_\__\__,_|_| |_| |_| \___/\__\___/__/__/\___/_|
#          |__/


#  ___                                   ___
# | _ \_ _ ___  __ ___ ______ ___ _ _   / _ \ _  _ ___ _  _ ___
# |  _/ '_/ _ \/ _/ -_|_-<_-</ _ \ '_| | (_) | || / -_) || / -_)
# |_| |_| \___/\__\___/__/__/\___/_|    \__\_\\_,_\___|\_,_\___|


#  _____     _    _       ___          _
# |_   _|_ _| |__| |___  | _ \___ __ _(_)___ _ _
#   | |/ _` | '_ \ / -_) |   / -_) _` | / _ \ ' \
#   |_|\__,_|_.__/_\___| |_|_\___\__, |_\___/_||_|
#                                |___/


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
        self.center += dt * self.velocity

    def draw(self):
        pygame.draw.circle(g_screen,
                           self.COLOR,
                           self.center.int_tuple,
                           self.RADIUS,
                           self.DONT_FILL_BIT)

    def step_many(self, steps, dt: float):
        self.center += steps * dt * self.velocity

    def predict_a_wall_collision(self, wall: Wall, dt):
        p = self.center
        q, t = collinear_point_and_parameter(wall.left, wall.right, p)
        contact_normal = (q - p).normalized()
        normal_component_of_velocity = \
            self.velocity.dot(contact_normal)
        contact_tangent = Vec2d(contact_normal[1], -contact_normal[0])
        tangential_component_of_velocity = \
            self.velocity.dot(contact_tangent)
        point_on_circle = p + self.RADIUS * contact_normal
        q_prime, t_prime = collinear_point_and_parameter(
            wall.left, wall.right, point_on_circle)
        # q_prime should be almost the same as q
        # TODO: np.testing.assert_allclose(...), meanwhile, inspect in debugger.
        projected_speed = self.velocity.dot(contact_normal)
        distance_to_wall = (q_prime - point_on_circle).length
        # predicted step time can be negative! it is permitted!
        predicted_step_time = distance_to_wall / projected_speed / dt \
            if projected_speed != 0 else np.inf
        return {'tau': predicted_step_time,
                'puck_strike_point': point_on_circle,
                'wall_strike_point': q_prime,
                'wall_strike_parameter': t_prime,
                'wall_victim': wall,
                'n': contact_normal,
                't': contact_tangent,
                'v_n': normal_component_of_velocity,
                'v_t': tangential_component_of_velocity}

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
        a = dv.get_length_sqrd()
        b = -2 * dp.dot(dv)
        d1 = self.RADIUS + them.RADIUS
        c = dp.get_length_sqrd() - (d1 * d1)
        disc = (b * b) - (4 * a * c)
        gonna_hit = False
        tau_impact_steps = np.inf
        if disc >= 0 and a != 0:
            # Two real roots; pick the smallest, non-negative one.
            sdisc = np.sqrt(disc)
            tau1 = (-b + sdisc) / (2 * a)
            tau2 = (-b - sdisc) / (2 * a)
            if tau1 >= 0 and tau2 >= 0:
                tau_impact_steps = min(tau1, tau2) / dt
                gonna_hit = True
            else:
                tau_impact_steps = max(tau1, tau2) / dt
                gonna_hit = tau1 >= 0 or tau2 >= 0
            # TODO: what does it mean if they're both negative? Is that even
            # possible?

        return {'tau': tau_impact_steps,
                'puck_victim': them,
                'gonna_hit': gonna_hit}


#   ___                   _              ___     _       _ _   _
#  / __|___ ___ _ __  ___| |_ _ _ _  _  | _ \_ _(_)_ __ (_) |_(_)_ _____ ___
# | (_ / -_) _ \ '  \/ -_)  _| '_| || | |  _/ '_| | '  \| |  _| \ V / -_|_-<
#  \___\___\___/_|_|_\___|\__|_|  \_, | |_| |_| |_|_|_|_|_|\__|_|\_/\___/__/
#                                 |__/


def parametric_line(p0: Vec2d, p1: Vec2d) -> Callable:
    """Returns a parametric function that produces all the points along an oriented
    line segment from point p0 to point p1 as the parameter varies from 0 to 1.
    Parameter values outside [0, 1] produce points along the infinite
    continuation of the line segment."""
    d = (p1 - p0).get_length()
    return lambda t: p0 + t * (p1 - p0) / d


def random_points(n):
    """Produces randomly chosen points inside the screen (TODO: possibly
    off-by-one on the right and the bottom)"""
    return [Vec2d(p[0] * SCREEN_WIDTH, p[1] * SCREEN_HEIGHT)
            for p in rndm.random((n, 2))]


def convex_hull(points: List[Vec2d]) -> List[Tuple[int, int]]:
    """Computes the convex hull of a set of 2D points.

    Input: an iterable sequence of Vec2d(x, y) representing the points. Output:
    a list of integer vertices of the convex hull in counter-clockwise order,
    starting from the vertex with the lexicographically smallest coordinates.
    Implements Andrew's monotone chain algorithm. O(n log n) complexity in
    time.

    See https://goo.gl/PR24H2 and https://goo.gl/x3dEM6"""

    # Sort the points lexicographically (tuples are compared lexicographically).
    # Remove duplicates to detect the case we have just one unique point.
    points = sorted(set([p.int_tuple for p in points]))

    # Boring case: no points or a single point, possibly repeated multiple times.
    if len(points) <= 1:
        return points

    # 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
    # Returns a positive value, if OAB makes a counter-clockwise turn,
    # negative for clockwise turn, and zero if the points are collinear.
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    # Build lower hull
    lower = []
    for p in points:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)

    # Build upper hull
    upper = []
    for p in reversed(points):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)

    # Concatenation of the lower and upper hulls gives the convex hull.
    # Last point of each list is omitted because it is repeated at the
    # beginning of the other list.
    return lower[:-1] + upper[:-1]


def collinear_point_and_parameter(u: Vec2d, v: Vec2d, p: Vec2d) -> \
        Tuple[Vec2d, float]:
    vmu_squared = (v - u).dot(v - u)
    t = (p - u).dot(v - u) / vmu_squared
    q = u + t * (v - u)
    return (q, t)


def rotate_seq(pt, c, s):
    x = pt[0]
    y = pt[1]
    return (c * x - s * y,
            s * x + c * y)


def translate_seq(pt, x_prime, y_prime):
    x = pt[0]
    y = pt[1]
    return (x + x_prime, y + y_prime)


def scale_seq(pt, xs, ys):
    x = pt[0]
    y = pt[1]
    return (x * xs, y * ys)


def centroid_seq(pts):
    l = len(pts)
    return (sum(pt[0] for pt in pts) / l,
            sum(pt[1] for pt in pts) / l)


#  ___             _   _               _   ___     _       _ _   _
# | __|  _ _ _  __| |_(_)___ _ _  __ _| | | _ \_ _(_)_ __ (_) |_(_)_ _____ ___
# | _| || | ' \/ _|  _| / _ \ ' \/ _` | | |  _/ '_| | '  \| |  _| \ V / -_|_-<
# |_| \_,_|_||_\__|\__|_\___/_||_\__,_|_| |_| |_| |_|_|_|_|_|\__|_|\_/\___/__/


def pairwise(ls, fn):
    result = []
    for i in range(len(ls) - 1):
        temp = fn(ls[i], ls[i + 1])
        result.append(temp)
    return result


def pairwise_toroidal(ls, fn):
    return pairwise(ls + [ls[0]], fn)


#  ___             _         _
# | _ \___ _ _  __| |___ _ _(_)_ _  __ _
# |   / -_) ' \/ _` / -_) '_| | ' \/ _` |
# |_|_\___|_||_\__,_\___|_| |_|_||_\__, |
#                                  |___/


def draw_int_tuples(int_tuples: List[Tuple[int, int]],
                    color=THECOLORS['yellow']):
    pygame.draw.polygon(g_screen, color, int_tuples, 1)


def draw_collinear_point_and_param(
        u=Vec2d(10, SCREEN_HEIGHT - 10 - 1),
        v=Vec2d(SCREEN_WIDTH - 10 - 1, SCREEN_HEIGHT - 10 - 1),
        p=Vec2d(SCREEN_WIDTH / 2 + DEMO_STEPS - 1,
                SCREEN_HEIGHT / 2 + (DEMO_STEPS - 1) / 2),
        point_color=THECOLORS['white'],
        line_color=THECOLORS['cyan']):
    dont_fill_bit = 0
    q, t = collinear_point_and_parameter(u, v, p)
    pygame.draw.circle(g_screen, point_color, p.int_tuple, SPOT_RADIUS,
                       dont_fill_bit)
    # pygame.draw.line(screen, point_color, q.int_tuple, q.int_tuple)
    pygame.draw.line(g_screen, line_color, p.int_tuple, q.int_tuple)


def draw_vector(p0: Vec2d, p1: Vec2d, color):
    pygame.draw.line(g_screen, color, p0.int_tuple, p1.int_tuple)
    pygame.draw.circle(g_screen, color, p1.int_tuple, SPOT_RADIUS, 0)


def draw_centered_arrow(loc, vel):
    arrow_surface = g_screen.copy()
    arrow_surface.set_alpha(175)

    arrow_pts = (
        (0, 100),
        (0, 200),
        (200, 200),
        (200, 300),
        (300, 150),
        (200, 0),
        (200, 100))

    speed = vel.length
    sps = [scale_seq(p, speed / 4, 1 / 4) for p in arrow_pts]

    ctr = centroid_seq(sps)
    cps = [translate_seq(p, -ctr[0], -ctr[1]) for p in sps]

    angle = np.arctan2(vel[1], vel[0])
    c = np.cos(angle)
    s = np.sin(angle)
    qs = [rotate_seq(p, c, s) for p in cps]

    ps = [translate_seq(p, loc[0], loc[1]) for p in qs]

    pygame.draw.polygon(
        arrow_surface,
        THECOLORS['white'],  # (0, 0, 0),
        ps,
        0)

    # ns = pygame.transform.rotate(arrow_surface, -angle)
    g_screen.blit(
        source=arrow_surface,
        dest=((0, 0)))  # ((loc - Vec2d(0, 150)).int_tuple))


def screen_cage():
    return [SCREEN_TL, SCREEN_BL, SCREEN_BR, SCREEN_TR]


def draw_cage():
    draw_int_tuples([p.int_tuple for p in screen_cage()], THECOLORS['green'])


def clear_screen(color=THECOLORS['black']):
    g_screen.fill(color)


#  ___
# |   \ ___ _ __  ___ ___
# | |) / -_) '  \/ _ (_-<
# |___/\___|_|_|_\___/__/


def demo_cage_time_warp(dt=1):
    """"""
    def default_event_main(
            vt: VirtualTime, state: State, msgs: List[EventMessage]):
        pass

    def default_query_main(
            vt: VirtualTime, state: State, msgs: List[EventMessage]):
        pass

    wall_lps = [WallLP(Wall(SCREEN_TL, SCREEN_BL), "left wall",
                       default_event_main, default_query_main),
                WallLP(Wall(SCREEN_BL, SCREEN_BR), "bottom wall",
                       default_event_main, default_query_main),
                WallLP(Wall(SCREEN_BR, SCREEN_TR), "right wall",
                       default_event_main, default_query_main),
                WallLP(Wall(SCREEN_TR, SCREEN_TL), "top wall",
                       default_event_main, default_query_main)]

    [w.wall.draw() for w in wall_lps]

    small_puck_velocity = Vec2d(2.3, -1.7)
    small_puck_lp = PuckLP(
        puck=Puck(
            center=Vec2d(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2),
            velocity=small_puck_velocity,
            mass=100,
            radius=42,
            color=THECOLORS['red']),
        me=ProcessID("small puck"),
        query_main=default_query_main)
    small_puck_lp.puck.draw()
    draw_centered_arrow(small_puck_lp.puck.center,
                        small_puck_lp.puck.velocity)

    big_puck_lp = PuckLP(
        puck=Puck(
            center=Vec2d(SCREEN_WIDTH / 1.5, SCREEN_HEIGHT / 2.5),
            velocity=Vec2d(-1.95, -0.20),
            mass=100,
            radius=79,
            color=THECOLORS['green']),
        me=ProcessID("big puck"),
        query_main=default_query_main)

    # boot the OS
    global sched_q
    sched_q = ScheduleQueue()
    for wall in wall_lps:
        sched_q.insert(wall)
    sched_q.insert(small_puck_lp)
    sched_q.insert(big_puck_lp)

    # boot the simulation:
    small_puck_lp.send(
        other=ProcessID('small puck'),
        receive_time=VirtualTime(0),
        body=Body({
            'center': small_puck_lp.puck.center,
            'velocity': small_puck_lp.puck.velocity,
            'walls': wall_lps,
            'dt': dt
        }),
        force_send_time=EARLIEST_VT)

    pygame.display.flip()


def demo_cage(pause=0.75, dt=1):
    me, them = mk_us()
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


def step_and_draw_both(dt, me, tau, them):
    me.step_many(int(tau), dt)
    them.step_many(int(tau), dt)
    me.draw()
    them.draw()


def wall_prediction(puck, walls, dt):
    predictions = \
        [puck.predict_a_wall_collision(wall, dt) for wall in walls]
    prediction = min(
        predictions,
        key = lambda p: p['tau'] if p['tau'] >= 0 else np.inf)
    return prediction


def draw_us_with_arrows(me, them):
    me.draw()
    draw_centered_arrow(loc=me.center, vel=me.velocity)
    them.draw()
    draw_centered_arrow(loc=them.center, vel=them.velocity)


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


def draw_perps_to_cage(puck: Puck):
    top_left = Vec2d(TOP_LEFT)
    bottom_left = Vec2d(BOTTOM_LEFT)
    bottom_right = Vec2d(BOTTOM_RIGHT)
    top_right = Vec2d(TOP_RIGHT)
    p = puck.center
    draw_collinear_point_and_param(bottom_left, bottom_right, p)
    draw_collinear_point_and_param(top_left, top_right, p)
    draw_collinear_point_and_param(top_left, bottom_left, p)
    draw_collinear_point_and_param(top_right, bottom_right, p)


def set_up_screen(pause=0.75):
    global g_screen
    pygame.init()
    g_screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    # clock = pygame.time.Clock()
    g_screen.set_alpha(None)
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
        mass=100)
    puck.step_many(DEMO_STEPS, DEMO_DT)
    hull = convex_hull(random_points(15))
    draw_int_tuples(hull)
    pairwise_toroidal(hull,
                      lambda p0, p1: draw_collinear_point_and_param(
                          Vec2d(p0), Vec2d(p1), line_color=THECOLORS['purple']))
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
        g_screen.fill(THECOLORS["black"])
        draw(g_screen, self.space)
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
    global g_screen
    set_up_screen()
    demo_cage_time_warp(dt=0.001)
    time.sleep(0.75)
    clear_screen()
    for _ in range(3):
        demo_cage(pause=0.75, dt=0.001)
        clear_screen()
    # demo_hull(0.75)
    # demo_classic(steps=3000)
    # input('Press [Enter] to end the program.')


if __name__ == "__main__":
    main()
