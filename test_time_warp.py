from colliding_pucks import *
import sortedcontainers
import pytest


from collections import namedtuple
from locutius.multimethods import multi, multimethod, method


from typing import Dict


from unittest import TestCase
import pytest


def test_named_tuple_type_equality():
    """Like each call of 'namedtuple' creates a new class, and, if they're
    named the same, the new one overwrites the old one. Instances still test
    equal, as the next test shows."""
    a = namedtuple('stuff', ['forty_two'])
    b = namedtuple('stuff', ['forty_two'])
    assert a is not b


def test_named_tuple_instance_equality():
    a = namedtuple('stuff', ['forty_two'])(forty_two=42)
    b = namedtuple('stuff', ['forty_two'])(forty_two=42)
    assert a == b


def test_event_message():

    with pytest.raises(ValueError):
        # receivetime strictly greater than sendtime
        EventMessage("me", 42, "it", 42, 1, {})

    msg = EventMessage("me", 42, "it", 43, 1, {})
    assert msg is not None, f"shorthand form is OK."

    msg = EventMessage(sender="me",
                       send_time=VirtualTime(0),
                       receiver="it",
                       receive_time=VirtualTime(1),
                       sign=True,
                       body=namedtuple('stuff', ['forty_two'])(forty_two=42))
    assert msg is not None, f"mixed shorthand is OK."

    msg2 = EventMessage(sender="me",
                        send_time=VirtualTime(0),
                        receiver="it",
                        receive_time=VirtualTime(1),
                        sign=False,
                        body=namedtuple('stuff', ['forty_two'])(forty_two=42))
    assert msg2 is not None

    assert msg == msg2, f"message equality doesn't depend on sign."

    msg3 = EventMessage(sender=ProcessID("me"),
                        send_time=VirtualTime(100),
                        receiver=ProcessID("it"),
                        receive_time=VirtualTime(150),
                        sign=False,
                        body=namedtuple('stuff', ['forty_two'])(forty_two=42))
    assert msg3 is not None, f"fullform is OK"

    assert msg3 > msg2, f"virtual-time strict comparison is OK."
    assert msg3 >= msg2, f"virtual-time non-strict comparison is OK."

    msg4 = EventMessage(sender=ProcessID("me"),
                        send_time=VirtualTime(100),
                        receiver=ProcessID("it"),
                        receive_time=VirtualTime(150),
                        sign=False,
                        body=namedtuple('sauce', ['kind'])(kind='Worcestershire'))

    assert not msg3 == msg4, f"message equality does depend on body."
    assert msg3 != msg4, f"message inequality operator if OK."


def test_twstate():
    state = State(sender=ProcessID("me"),
                  send_time=VirtualTime(100),
                  body=namedtuple('sauce', ['kind'])(kind='Steak'))
    assert state is not None

    state2 = State(sender=ProcessID("me"),
                   send_time=VirtualTime(180),
                   body=namedtuple('sauce', ['kind'])(kind='Heinz 57'))

    assert not state2 < state, f"state timestamp lt comparison is OK."
    assert state2 >= state, f"state timestamp ge is OK."
    assert state2 > state, f"state timestamp gt is OK."


# TODO: hypothesis testing


def test_twqueue():
    q = TWQueue()
    m = EventMessage("me", 100, "it", 150, True, {'dressing': 'caesar'})
    q.insert(m)
    debug_me = q.vts()
    assert debug_me == [150], f"insert into empty is OK"
    mm = EventMessage("me", 100, "it", 150, False, {'dressing': 'caesar'})
    mm.vt = mm.receive_time
    q.insert(mm)
    assert q.vts() == [], f"annihilation happens."
    assert q.annihilation, f"annihilation flag is set."
    q.insert(m)
    assert q.vts() == [150], f"re-insert into empty is OK"
    q.insert(EventMessage("me", 100, "alice", 150, True, {'dressing': 'mayo'}))
    assert q.vts() == [150]
    assert len(q.elements[150]) == 2


def test_input_queue():
    q = InputQueue()
    m = EventMessage("me", 100, "it", 150, True, {'dressing': 'caesar'})
    q.insert(m)
    assert q.vts() == [150], f"insert into empty is OK"
    mm = EventMessage("me", 100, "it", 150, False, {'dressing': 'caesar'})
    q.insert(mm)
    assert q.vts() == [], f"annihilation happens."
    assert q.annihilation, f"annihilation flag is set."


def test_bisections():

    bisection_playground()

    def em(rt: VirtualTime):
        return EventMessage('me', -42, 'it', rt, True, {})

    g = InputQueue()
    t10 = g.earliest_later_time(90)
    assert t10 == LATEST_VT
    t11 = g.latest_earlier_time(90)
    assert t11 == EARLIEST_VT

    g.insert(em(100))

    # Three cases: searched time is <, ==, > time of an element in the queue

    t12 = g.earliest_later_time(90)
    assert t12 == 100
    t13 = g.latest_earlier_time(90)
    assert t13 == EARLIEST_VT

    t14 = g.earliest_later_time(100)
    assert t14 == LATEST_VT
    t15 = g.latest_earlier_time(100)
    assert t15 == EARLIEST_VT

    t16 = g.earliest_later_time(120)
    assert t16 == LATEST_VT
    t17 = g.latest_earlier_time(120)
    assert t17 == 100

    g.insert(em(150))
    # Before any
    t18 = g.earliest_later_time(90)
    assert t18 == 100
    t19 = g.latest_earlier_time(90)
    assert t19 == EARLIEST_VT
    # Right on the first one
    t1a = g.earliest_later_time(100)
    assert t1a == 150
    t1b = g.latest_earlier_time(100)
    assert t1b == EARLIEST_VT
    # Between the two
    t1c = g.earliest_later_time(120)
    assert t1c == 150
    t1d = g.latest_earlier_time(120)
    assert t1d == 100
    # Right on the second one
    t1e = g.earliest_later_time(150)
    assert t1e == LATEST_VT
    t1f = g.latest_earlier_time(150)
    assert t1f == 100
    # After all of them
    t20 = g.earliest_later_time(180)
    assert t20 == LATEST_VT
    t21 = g.latest_earlier_time(180)
    assert t21 == 150

    assert g.vts() == [100, 150]
    g.insert(em(200))
    assert g.vts() == [100, 150, 200]
    g.remove(200)
    assert g.vts() == [100, 150]

    pass  # set debugger breakpoint here


def bisection_playground():
    # buncha stuff to inspect in debugger to suss out bisection routines.
    f = sortedcontainers.SortedDict({})
    f[100] = 'a'
    f[150] = 'b'
    f[200] = 'c'
    t1 = f.bisect_left(90)
    t2 = f.bisect_right(90)
    t11 = f.bisect_left(100)
    t12 = f.bisect_right(100)
    t3 = f.bisect_left(120)
    t4 = f.bisect_right(120)
    t5 = f.bisect_left(150)
    t6 = f.bisect_right(150)
    t9 = f.bisect_left(180)
    ta = f.bisect_right(180)
    tb = f.bisect_left(200)
    tc = f.bisect_right(200)
    t7 = f.bisect_left(900)
    t8 = f.peekitem(-1)
    t9 = f.bisect_right(900)
    pass


def test_keys():
    f = sortedcontainers.SortedDict({})
    f[100] = 'a'
    f[150] = 'b'
    f[200] = 'c'
    t0 = len(f)
    ks = f.keys()
    ls = list(ks)
    assert ls == [100, 150, 200]


def test_sorted_containers():
    # Test the underlying representation qua implementation:
    f = sortedcontainers.SortedDict({})
    assert f == {}
    assert f.bisect_left(150) == 0
    assert f.bisect_right(150) == 0
    f[100] = 'a'
    assert f.bisect_left(150) == 1
    assert f.bisect_right(150) == 1
    assert f.bisect_left(90) == 0
    assert f.bisect_right(90) == 0
    f[150] = 'b'
    f[200] = 'c'
    assert f.bisect_left(100) == 0
    assert f.bisect_right(100) == 1
    assert f.bisect_left(150) == 1
    assert f.bisect_right(150) == 2
    assert f.bisect_left(90) == 0
    assert f.bisect_right(90) == 0


def test_output_queue():
    pass


def test_sched_queue():
    pass
