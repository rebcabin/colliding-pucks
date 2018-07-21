from typing import List, Tuple, Callable, Dict, Any

import sortedcontainers

from constants import *
import globals

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
        while True:
            lvt, lps = earliest_item = self.elements.peekitem(0)

            # just run the first lp in the list returned by peekitem:
            input_bundle = lps[0].iq.elements[lvt]  # Let iq throw if no input
            # messages!
            levt = lps[0].sq.latest_earlier_time(lvt)
            states = lps[0].sq.elements.get(levt, {})
            assert len(states) == 1
            state = states[0]
            state_prime = lps[0].event_main(lvt, state, input_bundle)

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
        other_lp = globals.sched_q.world_map[other]
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
            lp_bundle = globals.sched_q.elements.pop(self.now)
            # find self in the bundle
            for i in range(len(lp_bundle)):
                if self is lp_bundle[i]:
                    pre = lp_bundle[:i]
                    post = lp_bundle[i + 1:]
                    me = lp_bundle[i]
                    residual = pre + post
                    globals.sched_q.insert_bundle(residual)
                    me.now = me.vt = new_lp_vt
                    globals.sched_q.insert(me)
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
