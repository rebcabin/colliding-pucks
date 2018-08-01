# colliding-pucks
Resurrecting the Time Warp Operating System and its Colliding Pucks benchmark.

To run this, use at least Python 3.6, and see 'requirements.txt' for the
libraries you'll need. Pymunk 4.0 is snapshotted here; find 'setup.py' in its
directory and type 'python setup.py install'. For the others, 'pip install' will
do, e.g., 'pip install numpy,' 'pip install pygame,' 'pip install pytest.'

To run unit tests, do 'pytest test_time_warp.py.'

To run the graphics demos, do 'python colliding_pucks.py.'

Known limitations:

* This is Time Warp on one processor for now. It just illustrates the
  fundamental queuing discipline, which is the cornerstone of the Time Warp
  mechanism, but isn't distributed YET!

* This is a demo for a talk . It has many ugly hacks and intentional shortcuts.
  I documented them meticulously with "TODO," but certainly missed some. It does
  not quite rise to the level of "proof of concept," and certainly not to
  "prototype." However, there is nothing fundamental in the way of improving it
  gradually to those levels, the queuing discipline being sound as I believe.

* Cancellation is not tested because this is not distributed.

* xxx fixed xxx
  [There is a momentum leak of unknown cause. The simulation
  eventually slows down (in simulation time) and the next
  event is scheduled infinitely far in the virtual future,
  causing premature termination. I'm working on it.]

  This was resolved and led to a bug with eager cancellation (work in progress).

* This uses eager cancellation (it's just a demo at this point). Lazy
  cancellation is necessary for good performance.

* A better method is to have table regions coordinate all events. I intend to
  get there some day.

* Use ROSS from Rensselaer (http://carothersc.github.io/ROSS/,
  https://github.com/carothersc/ROSS) for real work with Time Warp.
