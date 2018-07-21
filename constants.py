import sys

from pymunk.vec2d import Vec2d

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


