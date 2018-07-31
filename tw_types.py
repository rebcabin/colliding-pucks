from typing import List, Tuple, Callable, Dict, Any

import pprint
pp = pprint.PrettyPrinter(indent=2)

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
            t0 = 42 + 79  # set debugger breakpoint here
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


class QueryMessage(EventMessage):
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
        result = list(self.elements.keys())
        return result

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
        self.rollback = False
        if m.vt <= self.vt:
            self.rollback = True
            # Even if there is eventual annihilation, we need to roll
            # back to this time or earlier:
            # TODO: Do lazy cancellation at the end of each event.
            self.vt = m.vt
        if m.vt in self.elements:
            # search for the antimessage of this message:
            for e in self.elements[m.vt]:
                if (e == m and
                        hasattr(m, 'sign') and
                        hasattr(e, 'sign') and
                        e.sign == (not m.sign)):
                    self.annihilation = True
                    # TODO: merge this logic with that in 'send'
                    pp.pprint({
                        'annihilation': True,
                        'cause': vars(m)
                    })
                    self.elements[m.vt].remove(e)
                    # If there are no more timestamped's, kill the key and
                    # move the rollback time to latest earlier time.
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

    def insert(self, lp: 'LogicalProcess'):
        globals.world_map[lp.me] = lp
        super().insert(lp)

    def run(self, drawing=True, pause=0.0):
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
        while True:  # TODO: finite number of steps
            lvt, lps = self.elements.popitem(0)
            lp = lps[0]  # just run the first lp in the list
            self.insert_bundle(lps[1:])  # put the others back
            try: # Let iq throw if no input messages!
                input_bundle = lp.iq.elements[lvt]
            except KeyError as e:
                continue
            le_vt = lp.sq.latest_earlier_time(lvt)
            states = lp.sq.elements.get(le_vt, {})
            assert len(states) == 1
            state = states[0]
            lp.now = lvt

            # TODO: Query messages are handled synchronously, in-line.
            try:
                state_prime = lp.event_main(lvt, state, input_bundle)
            except Exception as e:
                print(f"likely process self-preemption: {e}")

            # TODO: Destructively overwrite state:
            # assert state_prime.vt == lvt
            if state is not state_prime:
                lp.sq.insert(state_prime)
            earliest_later_time = lp.iq.earliest_later_time(lp.now)

            # scoot the process forward; important to do it at this point in
            # the loop so that other elements can cause rollback anc
            # cancellation.
            lp.vt = earliest_later_time
            lp.iq.vt = earliest_later_time
            self.insert(lp)

            self.gvt_chores()

        pass

    def gvt_chores(self):
        gvt = sys.maxsize
        max_iq_length = 0
        max_oq_length = 0
        max_sq_length = 0
        for vt, lps in self.elements.items():
            gvt = min(gvt, vt)
            for lp in lps:
                max_iq_length = max(max_iq_length, len(lp.iq.elements))
                max_oq_length = max(max_oq_length, len(lp.oq.elements))
                max_sq_length = max(max_sq_length, len(lp.sq.elements))
        # print({'gvt': gvt,
        #        'max iq len': max_iq_length,
        #        'max oq len': max_oq_length,
        #        'max sq len': max_sq_length})


#  _              _         _   ___
# | |   ___  __ _(_)__ __ _| | | _ \_ _ ___  __ ___ ______
# | |__/ _ \/ _` | / _/ _` | | |  _/ '_/ _ \/ _/ -_|_-<_-<
# |____\___/\__, |_\__\__,_|_| |_| |_| \___/\__\___/__/__/
#           |___/


class LogicalProcess(Timestamped):
    def __init__(self,
                 me: ProcessID,
                 ):
        self.now = LATEST_VT
        super().__init__(self.now)
        self.iq = InputQueue()
        self.oq = OutputQueue()
        self.sq = StateQueue()
        self.me = me

    def event_main(self, lvt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        raise NotImplementedError

    def query_main(self, vt: VirtualTime, state: State,
                   msgs: List[EventMessage]):
        raise NotImplementedError

    def draw(self):
        raise NotImplementedError

    def new_state(self, body: Body):
        return State(sender=self.me,
                     send_time=self.now,
                     body=body)

    def send(self,
             rcvr_pid: ProcessID,
             receive_time: VirtualTime,
             body: Body,
             force_send_time=None):  # for boot only
        # TODO: Does the rest of this stuff belong in the Schedule Queue?
        rcvr_lp = globals.globals.world_map[rcvr_pid]
        msg, antimsg = self._message_pair(
            rcvr_pid, force_send_time, receive_time, body)
        self.oq.insert(antimsg)
        if self.oq.annihilation:
            # TODO: This won't be an error when flow-control by cancelback
            # TODO: is implemented.
            raise ValueError(
                f'unexpected annihilation from antimessage '
                f'{pp.pformat(antimsg)} at process '
                f'{pp.pformat(self)}')
        # TODO: When distributed, the other machine will do the following:
        rcvr_lp.iq.insert(msg)
        if rcvr_lp.iq.rollback:
            self._reschedule(rcvr_lp)
            # TODO: Eager cancellation:
            # TODO: If I'm cancelling output to myself, I must terminate my
            # TODO: current thread of execution. One good way to do that is
            # TODO: to raise an exception, which must be caught in the
            # TODO: scheduler-queue's "run" method.
            print({'rollback': 'in send', 'cause': vars(msg)})
            if not force_send_time:
                for ovt in rcvr_lp.oq.elements:
                    if ovt >= receive_time:
                        for amsg in rcvr_lp.oq.elements.pop(ovt):
                            receiver_pid = amsg.receiver
                            receiver_lp = globals.world_map[receiver_pid]
                            receiver_lp.iq.insert(amsg)

            rcvr_lp.iq.rollback = False

    def _reschedule(self, lp):
        new_lp_vt = lp.iq.vt
        if lp.vt in globals.sched_q.elements:
            lp_bundle = globals.sched_q.elements.pop(lp.vt)
            residual = lp_bundle  # for the case where self is not in lp_bundle
            # find self in the bundle and put everyone else back.
            # TODO: abstract the following operation into the queues
            for i in range(len(lp_bundle)):
                if lp is lp_bundle[i]:
                    pre = lp_bundle[:i]
                    post = lp_bundle[i + 1:]
                    residual = pre + post
                    lp.now = lp.vt = new_lp_vt
                    globals.sched_q.insert(lp)
                    break
            globals.sched_q.insert_bundle(residual)
        else:
            """I'm not in the scheduling queue, which means I've been popped 
            out of in in the scheduler loop. The scheduler will put me back."""
            pass

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

    def query(self, other_pid: ProcessID, body: Body):
        other_lp = globals.world_map[other_pid]
        other_le_vt = other_lp.sq.latest_earlier_time(self.now)
        other_states = other_lp.sq.elements[other_le_vt]
        assert len(other_states) == 1
        result = other_lp.query_main(
            self.now,
            other_states[0],
            QueryMessage(
                sender=self.me,
                send_time=self.now - 1,  # TODO: BAD HACK!
                receiver=other_pid,
                receive_time=self.now,
                sign=True,
                body=body))
        return result
