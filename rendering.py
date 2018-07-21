import numpy as np
import time

import pygame
from pygame.color import THECOLORS

from constants import *
import globals
from geometry import *

#  ___             _         _
# | _ \___ _ _  __| |___ _ _(_)_ _  __ _
# |   / -_) ' \/ _` / -_) '_| | ' \/ _` |
# |_|_\___|_||_\__,_\___|_| |_|_||_\__, |
#                                  |___/


def set_up_screen(pause=0.75):
    pygame.init()
    globals.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    # clock = pygame.time.Clock()
    globals.screen.set_alpha(None)
    time.sleep(pause)


def draw_int_tuples(int_tuples: List[Tuple[int, int]],
                    color=THECOLORS['yellow']):
    pygame.draw.polygon(globals.screen, color, int_tuples, 1)


def draw_collinear_point_and_param(
        u=Vec2d(10, SCREEN_HEIGHT - 10 - 1),
        v=Vec2d(SCREEN_WIDTH - 10 - 1, SCREEN_HEIGHT - 10 - 1),
        p=Vec2d(SCREEN_WIDTH / 2 + DEMO_STEPS - 1,
                SCREEN_HEIGHT / 2 + (DEMO_STEPS - 1) / 2),
        point_color=THECOLORS['white'],
        line_color=THECOLORS['cyan']):
    dont_fill_bit = 0
    q, t = collinear_point_and_parameter(u, v, p)
    pygame.draw.circle(globals.screen, point_color, p.int_tuple, SPOT_RADIUS,
                       dont_fill_bit)
    # pygame.draw.line(screen, point_color, q.int_tuple, q.int_tuple)
    pygame.draw.line(globals.screen, line_color, p.int_tuple, q.int_tuple)


def draw_vector(p0: Vec2d, p1: Vec2d, color):
    pygame.draw.line(globals.screen, color, p0.int_tuple, p1.int_tuple)
    pygame.draw.circle(globals.screen, color, p1.int_tuple, SPOT_RADIUS, 0)


def draw_centered_arrow(loc, vel):
    arrow_surface = globals.screen.copy()
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
    globals.screen.blit(
        source=arrow_surface,
        dest=((0, 0)))  # ((loc - Vec2d(0, 150)).int_tuple))


def screen_cage():
    return [SCREEN_TL, SCREEN_BL, SCREEN_BR, SCREEN_TR]


def draw_cage():
    draw_int_tuples([p.int_tuple for p in screen_cage()], THECOLORS['green'])


def clear_screen(color=THECOLORS['black']):
    globals.screen.fill(color)


