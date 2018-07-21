from typing import List, Tuple, Callable, Dict, Any

from pymunk.vec2d import Vec2d

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


