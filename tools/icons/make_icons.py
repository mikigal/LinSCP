#!/usr/bin/env python3
"""Generates LinSCP's icon theme (source/qt/icons/linscp): colorful flat
icons in the style of WinSCP's (yellow folders, white pages, green and blue
arrows, red crosses), drawn from scratch: WinSCP's own icons are not free to
reuse (license.txt, part B). The icons are named as the freedesktop icons the
GUI asks for; the names missing here come from the desktop's theme.

    make_icons.py [OUTPUT_DIR]   # default: source/qt/icons/linscp
    make_icons.py --sheet PNG    # a contact sheet of the icons (rsvg-convert, montage)
"""

import math
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# The palette
FOLDER_BACK = '#e9a93c'
FOLDER_FRONT = '#fbd35b'
FOLDER_STROKE = '#a8761a'
PAGE = '#ffffff'
PAGE_STROKE = '#6f7c8c'
PAGE_FOLD = '#d7dde5'
PAGE_LINE = '#a4b0bf'
BLUE = '#2f80d9'
BLUE_DARK = '#1b56a0'
BLUE_LIGHT = '#9fd0ff'
GREEN = '#3eaa3e'
GREEN_DARK = '#227525'
RED = '#e1423b'
RED_DARK = '#9e221d'
GRAY = '#9aa3ad'
GRAY_DARK = '#4e5762'
GRAY_LIGHT = '#dfe3e8'
ORANGE = '#f28c28'
ORANGE_DARK = '#b25a0c'
YELLOW = '#ffd33d'
YELLOW_DARK = '#b88a00'
SCREEN = '#253040'
PURPLE = '#8a5ad8'
BROWN = '#a8743c'
BROWN_DARK = '#6e4518'
AMBER = '#f9b928'
AMBER_DARK = '#a8720a'
PAGE_FADED = '#b9c1cb'


def svg(*parts):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">\n' +
            '\n'.join(parts) + '\n</svg>\n')


def group(transform, *parts):
    return '<g transform="%s">%s</g>' % (transform, ''.join(parts))


def path(d, fill='none', stroke=None, width=1, extra=''):
    s = '<path d="%s" fill="%s"' % (d, fill)
    if stroke:
        s += ' stroke="%s" stroke-width="%g" stroke-linejoin="round" stroke-linecap="round"' % (stroke, width)
    return s + extra + '/>'


def rect(x, y, w, h, fill, stroke=None, rx=0, width=1):
    s = '<rect x="%g" y="%g" width="%g" height="%g" fill="%s"' % (x, y, w, h, fill)
    if rx:
        s += ' rx="%g"' % rx
    if stroke:
        s += ' stroke="%s" stroke-width="%g"' % (stroke, width)
    return s + '/>'


def circle(cx, cy, r, fill, stroke=None, width=1):
    s = '<circle cx="%g" cy="%g" r="%g" fill="%s"' % (cx, cy, r, fill)
    if stroke:
        s += ' stroke="%s" stroke-width="%g"' % (stroke, width)
    return s + '/>'


def line(x1, y1, x2, y2, stroke, width=1):
    return ('<line x1="%g" y1="%g" x2="%g" y2="%g" stroke="%s" stroke-width="%g" stroke-linecap="round"/>' %
            (x1, y1, x2, y2, stroke, width))


def text(x, y, s, fill, size=7, weight='bold'):
    return ('<text x="%g" y="%g" font-family="DejaVu Sans, sans-serif" font-size="%g" font-weight="%s" '
            'fill="%s" text-anchor="middle">%s</text>' % (x, y, size, weight, fill, s))


# Shapes ---------------------------------------------------------------------

def folder(open_=False):
    back = path('M1.5,3.5 h4.2 l1.4,1.5 h7.4 v8.5 h-13 z', FOLDER_BACK, FOLDER_STROKE)
    if open_:
        front = path('M3,7.5 h12 l-2,6 h-12 z', FOLDER_FRONT, FOLDER_STROKE)
    else:
        front = path('M1.5,6.5 h13 v7 h-13 z', FOLDER_FRONT, FOLDER_STROKE)
    return back + front


def page(x=3.5, y=1.5, w=9, h=13, lines=True):
    fold = 3
    s = path('M%g,%g h%g l%g,%g v%g h%g z' % (x, y, w - fold, fold, fold, h - fold, -w), PAGE, PAGE_STROKE)
    s += path('M%g,%g v%g h%g' % (x + w - fold, y, fold, fold), PAGE_FOLD, PAGE_STROKE)
    if lines:
        for i in range(int((h - 5) / 2)):
            ly = y + 5 + 2 * i
            s += line(x + 2, ly, x + w - 2, ly, PAGE_LINE)
    return s


def arrow(direction, cx, cy, size, fill, stroke, width=1):
    # A block arrow pointing to the direction (up, down, left, right, or the
    # degrees clockwise from up)
    h = size / 2.0
    t = size * 0.22
    pts = [(0, -h), (h, 0), (t, 0), (t, h), (-t, h), (-t, 0), (-h, 0)]
    angle = {'up': 0, 'right': 90, 'down': 180, 'left': 270}.get(direction, direction)
    a = math.radians(angle)
    out = []
    for px, py in pts:
        rx = px * math.cos(a) - py * math.sin(a)
        ry = px * math.sin(a) + py * math.cos(a)
        out.append('%.2f,%.2f' % (cx + rx, cy + ry))
    return '<polygon points="%s" fill="%s" stroke="%s" stroke-width="%g" stroke-linejoin="round"/>' % (
        ' '.join(out), fill, stroke, width)


def arrow_head(x, y, dx, dy, size, fill, stroke, width=1):
    # The head of a curved arrow ending at x, y in the direction dx, dy
    n = math.hypot(dx, dy)
    ux, uy = dx / n, dy / n
    tip = (x + ux * size, y + uy * size)
    p1 = (x - uy * size, y + ux * size)
    p2 = (x + uy * size, y - ux * size)
    return '<polygon points="%.2f,%.2f %.2f,%.2f %.2f,%.2f" fill="%s" stroke="%s" stroke-width="%g" ' \
           'stroke-linejoin="round"/>' % (p1[0], p1[1], tip[0], tip[1], p2[0], p2[1], fill, stroke, width)


def chevron(direction, cx, cy, size, stroke, width=2):
    h = size / 2.0
    if direction == 'left':
        d = 'M%g,%g L%g,%g L%g,%g' % (cx + h / 2, cy - h, cx - h / 2, cy, cx + h / 2, cy + h)
    elif direction == 'right':
        d = 'M%g,%g L%g,%g L%g,%g' % (cx - h / 2, cy - h, cx + h / 2, cy, cx - h / 2, cy + h)
    elif direction == 'up':
        d = 'M%g,%g L%g,%g L%g,%g' % (cx - h, cy + h / 2, cx, cy - h / 2, cx + h, cy + h / 2)
    else:
        d = 'M%g,%g L%g,%g L%g,%g' % (cx - h, cy - h / 2, cx, cy + h / 2, cx + h, cy - h / 2)
    return path(d, 'none', stroke, width)


def badge(kind, cx=11.5, cy=11.5, r=3.5):
    if kind == 'plus':
        return circle(cx, cy, r, GREEN, GREEN_DARK) + line(cx - 1.8, cy, cx + 1.8, cy, '#fff', 1.4) + \
            line(cx, cy - 1.8, cx, cy + 1.8, '#fff', 1.4)
    if kind == 'minus':
        return circle(cx, cy, r, RED, RED_DARK) + line(cx - 1.8, cy, cx + 1.8, cy, '#fff', 1.4)
    if kind == 'x':
        return circle(cx, cy, r, RED, RED_DARK) + line(cx - 1.4, cy - 1.4, cx + 1.4, cy + 1.4, '#fff', 1.3) + \
            line(cx + 1.4, cy - 1.4, cx - 1.4, cy + 1.4, '#fff', 1.3)
    if kind == 'star':
        return star(cx, cy, r + 0.6)
    if kind == 'check':
        return circle(cx, cy, r, GREEN, GREEN_DARK) + path(
            'M%g,%g l1.3,1.3 l2.4,-2.6' % (cx - 1.9, cy), 'none', '#fff', 1.3)
    raise ValueError(kind)


def star(cx, cy, r):
    pts = []
    for i in range(10):
        a = math.pi / 2 + i * math.pi / 5
        rr = r if i % 2 == 0 else r * 0.45
        pts.append('%.2f,%.2f' % (cx + rr * math.cos(a), cy - rr * math.sin(a)))
    return '<polygon points="%s" fill="%s" stroke="%s" stroke-width="0.8" stroke-linejoin="round"/>' % (
        ' '.join(pts), YELLOW, YELLOW_DARK)


def refresh_arrows(cx=8, cy=8, r=5, color=GREEN, dark=GREEN_DARK):
    # Two arcs with their arrowheads
    s = ''
    for start in (200, 20):
        a1 = math.radians(start)
        a2 = math.radians(start + 130)
        x1, y1 = cx + r * math.cos(a1), cy - r * math.sin(a1)
        x2, y2 = cx + r * math.cos(a2), cy - r * math.sin(a2)
        s += path('M%.2f,%.2f A%g,%g 0 0 0 %.2f,%.2f' % (x1, y1, r, r, x2, y2), 'none', dark, 3.2)
        s += path('M%.2f,%.2f A%g,%g 0 0 0 %.2f,%.2f' % (x1, y1, r, r, x2, y2), 'none', color, 1.8)
        # The head at the end of the arc
        ah = a2 + math.radians(90)
        hx, hy = x2, y2
        p1 = (hx + 2.6 * math.cos(ah - 1.9), hy - 2.6 * math.sin(ah - 1.9))
        p2 = (hx + 2.6 * math.cos(ah + 1.9), hy - 2.6 * math.sin(ah + 1.9))
        tip = (hx + 2.2 * math.cos(ah), hy - 2.2 * math.sin(ah))
        s += '<polygon points="%.2f,%.2f %.2f,%.2f %.2f,%.2f" fill="%s" stroke="%s" stroke-width="0.8" ' \
             'stroke-linejoin="round"/>' % (p1[0], p1[1], tip[0], tip[1], p2[0], p2[1], color, dark)
    return s


def magnifier(cx=6.5, cy=6.5, r=4):
    return (line(cx + r * 0.7, cy + r * 0.7, 14, 14, GRAY_DARK, 2.6) +
            circle(cx, cy, r, BLUE_LIGHT, GRAY_DARK, 1.4) +
            path('M%g,%g a%g,%g 0 0 1 %g,%g' % (cx - 2, cy - 0.5, 2.2, 2.2, 1.6, -1.8), 'none', '#ffffff', 1))


def pencil(x=0, y=0):
    body = path('M4,13.5 l-1,-0 l0.6,-2.4 l7.8,-7.8 l1.8,1.8 l-7.8,7.8 z', YELLOW, YELLOW_DARK)
    tip = path('M3.6,11.1 l1.8,1.8 l-2.4,0.6 z', '#f3d7b5', GRAY_DARK, 0.8)
    eraser = path('M11.4,3.3 l1,-1 a0.9,0.9 0 0 1 1.3,0 l0.5,0.5 a0.9,0.9 0 0 1 0,1.3 l-1,1 z', '#f08a9a', '#a3485a', 0.8)
    return group('translate(%g,%g)' % (x, y), body, tip, eraser)


def gear(cx=8, cy=8, r=5.5, fill=GRAY, stroke=GRAY_DARK, teeth=8):
    pts = []
    for i in range(teeth * 2):
        a = i * math.pi / teeth
        rr = r if i % 2 == 0 else r * 0.78
        a1 = a - math.pi / (teeth * 2.6)
        a2 = a + math.pi / (teeth * 2.6)
        pts.append('%.2f,%.2f' % (cx + rr * math.cos(a1), cy + rr * math.sin(a1)))
        pts.append('%.2f,%.2f' % (cx + rr * math.cos(a2), cy + rr * math.sin(a2)))
    return ('<polygon points="%s" fill="%s" stroke="%s" stroke-width="1" stroke-linejoin="round"/>' % (
        ' '.join(pts), fill, stroke) + circle(cx, cy, r * 0.34, '#ffffff', stroke))


def monitor(screen=SCREEN):
    return (rect(1.5, 2.5, 13, 9, GRAY_LIGHT, GRAY_DARK, 1) + rect(3, 4, 10, 6, screen) +
            path('M6,14.5 h4 l-0.5,-3 h-3 z', GRAY, GRAY_DARK))


def terminal():
    return (rect(1.5, 2.5, 13, 11, SCREEN, '#111820', 1.2) + rect(1.5, 2.5, 13, 2.2, GRAY, GRAY_DARK) +
            path('M4,7.5 l2,1.8 l-2,1.8', 'none', '#7CFC72', 1.3) + line(7.5, 11.2, 10.5, 11.2, '#7CFC72', 1.3))


def house(x=0, y=0):
    return group('translate(%g,%g)' % (x, y),
                 rect(3.5, 7.5, 9, 7, '#f6ecd6', BROWN_DARK),
                 path('M1.5,8.5 L8,2 L14.5,8.5', '#ffffff00', RED_DARK, 2.4),
                 path('M1.5,8.5 L8,2 L14.5,8.5', 'none', RED, 1.4),
                 rect(6.5, 10, 3, 4.5, BROWN, BROWN_DARK))


def server():
    s = ''
    for i, y in enumerate((1.5, 6, 10.5)):
        s += rect(2.5, y, 11, 4, '#c8d2dc', GRAY_DARK, 0.8)
        s += circle(11, y + 2, 0.9, GREEN if i < 2 else ORANGE)
        s += line(4.5, y + 2, 8, y + 2, GRAY_DARK, 1)
    return s


def globe(cx=8, cy=8, r=6):
    return (circle(cx, cy, r, '#4aa3f0', BLUE_DARK) +
            path('M4.5,5 c1.5,-1 3,0 3,1.5 c0,1.5 -2,1 -2.5,2.5 c-0.4,1.2 -1.6,0.5 -2,-0.5 z', '#5cc35c', GREEN_DARK, 0.6) +
            path('M9,9 c1,-1 3,-0.5 3.5,0.8 c0.3,1 -1,2.6 -2.5,2.8 c-1,0 -1.8,-2 -1,-3.6 z', '#5cc35c', GREEN_DARK, 0.6) +
            path('M%g,%g a%g,%g 0 0 1 0,%g' % (cx, cy - r, r * 0.45, r, 2 * r), 'none', '#ffffff88', 0.8) +
            line(cx - r, cy, cx + r, cy, '#ffffff88', 0.8))


def padlock(open_=False, x=0, y=0):
    shackle = 'M5,7.5 v-2.5 a3,3 0 0 1 6,0' + (' v-0.5' if open_ else ' v2.5')
    if open_:
        shackle = 'M5,7.5 v-2.5 a3,3 0 0 1 6,0 v0.5'
    return group('translate(%g,%g)' % (x, y), path(shackle, 'none', GRAY_DARK, 1.8),
                 rect(3.5, 7.5, 9, 7, YELLOW, YELLOW_DARK, 1), rect(7.3, 9.5, 1.4, 3, YELLOW_DARK))


def funnel(fill=BLUE, stroke=BLUE_DARK):
    return path('M1.5,2.5 h13 l-5,6 v5.5 l-3,1.5 v-7 z', fill, stroke)


def list_lines(bullet=BLUE):
    s = rect(1.5, 1.5, 13, 13, '#ffffff', PAGE_STROKE, 1)
    for i, y in enumerate((4.5, 8, 11.5)):
        s += rect(3.5, y - 1, 2, 2, bullet)
        s += line(7, y, 12.5, y, PAGE_LINE, 1.4)
    return s


def person(cx=8, scale=1.0, fill=BLUE, stroke=BLUE_DARK):
    return (circle(cx, 5, 2.8 * scale, '#f6c9a3', '#b77d52', 0.8) +
            path('M%g,15 a%g,%g 0 0 1 %g,0 z' % (cx - 5 * scale, 5 * scale, 5 * scale, 10 * scale), fill, stroke))


def drive(removable=False):
    if removable:
        return (rect(4.5, 1.5, 7, 13, '#c8d2dc', GRAY_DARK, 1) + rect(6, 1.5, 4, 3, GRAY, GRAY_DARK) +
                circle(8, 11.5, 1, GREEN))
    return (path('M1.5,9.5 l2,-6 h9 l2,6 z', '#dde3ea', GRAY_DARK) + rect(1.5, 9.5, 13, 4, '#c8d2dc', GRAY_DARK, 0.8) +
            circle(12, 11.5, 0.9, GREEN) + line(3.5, 11.5, 8, 11.5, GRAY_DARK))


def tab_shape(fill='#ffffff'):
    # A window with its tab on the top
    return (path('M1.5,5.5 v-2 a1,1 0 0 1 1,-1 h4 a1,1 0 0 1 1,1 v2', BLUE, BLUE_DARK) +
            rect(1.5, 5.5, 13, 9, fill, GRAY_DARK, 1) + rect(1.5, 5.5, 13, 2, BLUE_LIGHT, GRAY_DARK, 1))


def floppy(x=0, y=0, fill=BLUE, stroke=BLUE_DARK):
    return group('translate(%g,%g)' % (x, y),
                 path('M1.5,1.5 h10 l3,3 v10 h-13 z', fill, stroke),
                 rect(4, 1.5, 6, 4, '#e8eef5', stroke, 0, 0.8), rect(7.5, 2.3, 1.5, 2.4, stroke),
                 rect(3.5, 8.5, 9, 6, '#ffffff', stroke, 0, 0.8))


def clipboard():
    return (rect(2.5, 2.5, 11, 12, BROWN, BROWN_DARK, 1) + rect(4.5, 4.5, 7, 8.5, '#ffffff', PAGE_STROKE, 0, 0.8) +
            rect(5.5, 1.5, 5, 2.5, GRAY, GRAY_DARK, 0.8) + line(6, 7, 10, 7, PAGE_LINE) + line(6, 9, 10, 9, PAGE_LINE) +
            line(6, 11, 9, 11, PAGE_LINE))


def calendar():
    s = rect(1.5, 2.5, 13, 12, '#ffffff', PAGE_STROKE, 1) + rect(1.5, 2.5, 13, 3.5, RED, RED_DARK, 1)
    for x in (4.5, 8, 11.5):
        for y in (8.5, 11.5):
            s += rect(x - 1, y - 1, 2, 2, PAGE_LINE)
    return s + line(5, 1.5, 5, 4, GRAY_DARK, 1.4) + line(11, 1.5, 11, 4, GRAY_DARK, 1.4)


def circle_sign(fill, stroke, sign):
    s = circle(8, 8, 6.5, fill, stroke)
    if sign == 'i':
        s += circle(8, 4.8, 1.1, '#ffffff') + rect(7, 6.8, 2, 5.2, '#ffffff', None, 0.6)
    elif sign == '?':
        s += path('M5.8,6 a2.3,2.3 0 1 1 3.4,2 c-0.8,0.5 -1.2,0.9 -1.2,1.9', 'none', '#ffffff', 1.8)
        s += circle(8, 12.3, 1.1, '#ffffff')
    elif sign == '!':
        s += rect(7, 3.5, 2, 6, '#ffffff', None, 0.6) + circle(8, 12, 1.1, '#ffffff')
    elif sign == 'x':
        s += line(5.3, 5.3, 10.7, 10.7, '#ffffff', 2) + line(10.7, 5.3, 5.3, 10.7, '#ffffff', 2)
    return s


def tree_nodes():
    return (line(4, 4, 4, 12, GRAY_DARK) + line(4, 8, 8, 8, GRAY_DARK) + line(4, 12, 8, 12, GRAY_DARK) +
            rect(1.5, 1.5, 5, 4, FOLDER_FRONT, FOLDER_STROKE, 0.5) + rect(7.5, 6, 6, 4, FOLDER_FRONT, FOLDER_STROKE, 0.5) +
            rect(7.5, 10, 6, 4, FOLDER_FRONT, FOLDER_STROKE, 0.5))


def panes():
    return (rect(1.5, 2.5, 13, 11, '#ffffff', GRAY_DARK, 1) + rect(1.5, 2.5, 13, 2.5, BLUE, BLUE_DARK, 1) +
            line(8, 5, 8, 13.5, GRAY_DARK) + line(3, 7, 6.5, 7, PAGE_LINE) + line(3, 9, 6.5, 9, PAGE_LINE) +
            line(9.5, 7, 13, 7, PAGE_LINE) + line(9.5, 9, 13, 9, PAGE_LINE))


def plug(connected=True):
    s = (path('M9.5,6.5 l-3,3 a3,3 0 0 0 4.2,4.2 l3,-3 z', GRAY, GRAY_DARK) + line(11, 5, 13, 3, GRAY_DARK, 1.4) +
         line(9.5, 6.5, 12.3, 3.7, GRAY_DARK, 1.2) + line(13.3, 8.3, 11.5, 10, GRAY_DARK, 1.2) +
         line(6.5, 13.5, 2.5, 13.5, GRAY_DARK, 1.6))
    if connected:
        s += circle(4.5, 5, 3, GREEN, GREEN_DARK) + path('M3.2,5 l1,1 l2,-2', 'none', '#ffffff', 1.1)
    else:
        s += circle(4.5, 5, 3, RED, RED_DARK) + line(3.4, 3.9, 5.6, 6.1, '#ffffff', 1.1) + \
            line(5.6, 3.9, 3.4, 6.1, '#ffffff', 1.1)
    return s


def link_chain():
    return (rect(1.5, 6, 7, 4, 'none', BLUE_DARK, 2, 1.8) + rect(7.5, 6, 7, 4, 'none', BLUE, 2, 1.8))


def palette():
    return (path('M8,1.5 a6.5,6.5 0 1 0 0,13 c1,0 1.5,-0.7 1.2,-1.6 c-0.4,-1.1 0.4,-1.9 1.4,-1.9 h1.4 '
                 'a2.5,2.5 0 0 0 2.5,-2.5 c0,-3.8 -2.9,-7 -6.5,-7 z', '#f3e3c4', BROWN_DARK) +
            circle(4.5, 6.5, 1.3, RED) + circle(7, 4, 1.3, YELLOW) + circle(10.5, 4.5, 1.3, GREEN) +
            circle(4.5, 10, 1.3, BLUE))


def power():
    return (path('M5,4.2 a5.2,5.2 0 1 0 6,0', 'none', RED_DARK, 3) + path('M5,4.2 a5.2,5.2 0 1 0 6,0', 'none', RED, 1.8) +
            line(8, 1.5, 8, 7.5, RED_DARK, 3) + line(8, 1.5, 8, 7.5, RED, 1.8))


def book():
    return (path('M2.5,2.5 h5 a1,1 0 0 1 1,1 v11 a1,1 0 0 0 -1,-1 h-5 z', BLUE, BLUE_DARK) +
            path('M13.5,2.5 h-5 a1,1 0 0 0 -1,1 v11 a1,1 0 0 1 1,-1 h5 z', BLUE_LIGHT, BLUE_DARK) +
            text(11, 10.5, '?', BLUE_DARK, 6))


def bookmark(fill=RED, stroke=RED_DARK):
    return path('M4.5,1.5 h7 v13 l-3.5,-3 l-3.5,3 z', fill, stroke)


def door_exit():
    return (rect(2.5, 1.5, 8, 13, BROWN, BROWN_DARK, 0.5) + circle(8.5, 8.5, 0.8, YELLOW) +
            arrow('right', 12, 8, 6, GREEN, GREEN_DARK))


def scissors():
    return (line(5, 2, 10.5, 10.5, GRAY_DARK, 1.4) + line(11, 2, 5.5, 10.5, GRAY_DARK, 1.4) +
            circle(4.5, 12, 2.2, 'none', RED, 1.4) + circle(11.5, 12, 2.2, 'none', RED, 1.4))


def curved_arrow(mirror=False, color=BLUE, dark=BLUE_DARK):
    s = (path('M12.5,12.5 v-3 a4,4 0 0 0 -4,-4 h-4', 'none', dark, 3.2) +
         path('M12.5,12.5 v-3 a4,4 0 0 0 -4,-4 h-4', 'none', color, 1.8) +
         '<polygon points="1.5,5.5 5,2.2 5,8.8" fill="%s" stroke="%s" stroke-width="0.8" stroke-linejoin="round"/>' %
         (color, dark))
    return group('translate(16,0) scale(-1,1)', s) if mirror else s


def broom():
    return (line(12.5, 1.5, 7.5, 7.5, BROWN_DARK, 1.8) +
            path('M6,6 l4,4 l-3,4.5 l-5.5,-5.5 z', YELLOW, YELLOW_DARK) + line(4, 10, 6.5, 12.5, YELLOW_DARK, 0.8) +
            line(5.5, 8.5, 8, 11, YELLOW_DARK, 0.8))


def key():
    return (circle(5, 6, 3.5, YELLOW, YELLOW_DARK) + circle(4.2, 5.2, 1, '#ffffff') +
            path('M7.5,8 l6,6 m-2,-2 l1.5,-1.5 m-3,0 l1.2,-1.2', 'none', YELLOW_DARK, 2) +
            path('M7.5,8 l6,6 m-2,-2 l1.5,-1.5 m-3,0 l1.2,-1.2', 'none', YELLOW, 1))


def sort_az(arrow_down=True):
    return (text(5, 7, 'A', BLUE_DARK, 7) + text(5, 15, 'Z', BLUE_DARK, 7) +
            arrow('down' if arrow_down else 'up', 12, 8, 9, GREEN, GREEN_DARK))


def duplicate_windows():
    return (rect(4.5, 1.5, 10, 8, '#ffffff', GRAY_DARK, 1) + rect(4.5, 1.5, 10, 2.5, BLUE, BLUE_DARK, 1) +
            rect(1.5, 6.5, 10, 8, '#ffffff', GRAY_DARK, 1) + rect(1.5, 6.5, 10, 2.5, BLUE, BLUE_DARK, 1))


def queue_list():
    s = rect(1.5, 1.5, 13, 13, '#ffffff', PAGE_STROKE, 1)
    for y, c in ((4.5, GREEN), (8, ORANGE), (11.5, GRAY)):
        s += arrow('right', 4.5, y, 3.4, c, GRAY_DARK)
        s += line(7.5, y, 12.5, y, PAGE_LINE, 1.4)
    return s


def gradient(name, top, bottom, x2=0, y2=1):
    return ('<linearGradient id="%s" x1="0" y1="0" x2="%g" y2="%g"><stop offset="0" stop-color="%s"/>'
            '<stop offset="1" stop-color="%s"/></linearGradient>' % (name, x2, y2, top, bottom))


def app_icon():
    # LinSCP: a padlock (the secure connection) with the transfers, as
    # upstream's icon says it, drawn in LinSCP's convention: the upload blue
    # up and right behind the lock, the download green down and left before
    # it; on a 64-pixel grid, readable down to 16 pixels
    defs = '<defs>%s%s%s%s</defs>' % (
        gradient('upload', '#6fd6ff', '#1565c8', 1, 1),
        gradient('download', '#a6e852', '#2f8f14', 1, 1),
        gradient('body', '#fbfcfd', '#a9b2bc'),
        gradient('shackle', '#f2f4f6', '#9aa4ae', 1, 0))
    # The lock in the middle, plain as upstream's
    shackle = 'M22.5,32 V22.5 a9.5,9.5 0 0 1 19,0 V32'
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64">\n' +
            defs + '\n<g transform="translate(32,32) scale(1.12) translate(-32,-32)">\n' +
            '\n'.join([
                arrow(45, 46.5, 17.5, 31, 'url(#upload)', '#0f4f9e', 2),
                path(shackle, 'none', '#56606b', 9),
                path(shackle, 'none', 'url(#shackle)', 5.5),
                rect(15, 30, 34, 25, 'url(#body)', '#56606b', 4.5, 2),
                path('M18.5,34.3 h27', 'none', '#ffffffcc', 1.5),
                arrow(225, 17.5, 46.5, 31, 'url(#download)', '#2a6e12', 2),
            ]) + '\n</g>\n</svg>\n')


# The icons ------------------------------------------------------------------

ICONS = {
    # Folders and places
    'folder': svg(folder()),
    'folder-open': svg(folder(True)),
    # The directories of the panels (their MIME type)
    'inode-directory': svg(folder()),
    'folder-new': svg(folder(), badge('star', 12, 4.5, 3.2)),
    'folder-documents': svg(folder(), page(6.5, 4, 6, 8, False), line(8, 8, 11, 8, PAGE_LINE), line(8, 10, 11, 10, PAGE_LINE)),
    'folder-remote': svg(folder(), group('translate(6,6) scale(0.6)', globe())),
    'folder-root': svg(folder(), text(8, 12.6, '/', FOLDER_STROKE, 7)),
    'folder-sync': svg(folder(), group('translate(4.5,4.5) scale(0.7)', refresh_arrows(8, 8, 4.6, BLUE, BLUE_DARK))),
    'document-open-folder': svg(folder(True)),
    'user-home': svg(house()),
    'go-home': svg(house()),
    'user-desktop': svg(monitor('#4aa3f0'), rect(4, 5, 3, 2, '#ffffffaa'), rect(8.5, 5, 3, 2, '#ffffffaa')),
    'drive-harddisk': svg(drive()),
    'drive-removable-media': svg(drive(True)),
    'network-server': svg(server()),
    'internet-web-browser': svg(globe()),
    'bookmarks': svg(bookmark()),
    'bookmark-new': svg(bookmark(), badge('star', 11.5, 11.5, 3)),
    # Files
    'document-new': svg(page(), badge('star', 4, 4, 3)),
    'document-open': svg(folder(True), group('translate(5,1) scale(0.55)', page(lines=False))),
    'document-edit': svg(page(2.5, 1.5, 9, 13), pencil(2, 0)),
    'document-properties': svg(page(2.5, 1.5, 9, 13), group('translate(6,6) scale(0.6)', gear(8, 8, 6))),
    'document-save': svg(floppy()),
    'document-save-all': svg(group('translate(3,-1) scale(0.8)', floppy()), group('translate(0,3) scale(0.8)', floppy())),
    'document-multiple': svg(page(5.5, 1.5, 9, 11, False), page(2.5, 3.5, 9, 11)),
    'document-compareleft': svg(page(1.5, 2.5, 6, 11, True), page(8.5, 2.5, 6, 11, True)),
    'documentinfo': svg(page(2.5, 1.5, 9, 13), group('translate(7,7) scale(0.56)', circle_sign(BLUE, BLUE_DARK, 'i'))),
    'edit-copy': svg(page(5.5, 1.5, 9, 11, False), page(1.5, 4.5, 9, 11)),
    'edit-cut': svg(scissors()),
    'edit-paste': svg(clipboard()),
    'edit-delete': svg(circle_sign(RED, RED_DARK, 'x')),
    'edit-delete-shred': svg(page(), badge('x', 11.5, 11.5, 3.5)),
    'edit-rename': svg(rect(1.5, 5.5, 13, 6, '#ffffff', GRAY_DARK, 1), line(4, 8.5, 8, 8.5, BLUE, 1.4),
                       line(10, 3.5, 10, 13.5, GRAY_DARK, 1.2), line(8.5, 3.5, 11.5, 3.5, GRAY_DARK),
                       line(8.5, 13.5, 11.5, 13.5, GRAY_DARK)),
    'edit-find': svg(magnifier()),
    'edit-find-replace': svg(magnifier(5.5, 5.5, 3.5), arrow('right', 12, 12, 5, GREEN, GREEN_DARK)),
    'edit-undo': svg(curved_arrow()),
    'edit-redo': svg(curved_arrow(True)),
    'edit-select': svg(rect(1.5, 1.5, 13, 13, 'none', BLUE, 0, 1.2).replace('/>', ' stroke-dasharray="2 1.5"/>'),
                       path('M5,5 l6,2.5 l-2.5,1 l2.5,3 l-1,1 l-2.5,-3 l-1.5,2 z', '#ffffff', GRAY_DARK)),
    'edit-select-all': svg(rect(1.5, 1.5, 13, 13, BLUE_LIGHT, BLUE, 0, 1.2).replace('/>', ' stroke-dasharray="2 1.5"/>'),
                           line(4, 5, 12, 5, BLUE_DARK, 1.4), line(4, 8, 12, 8, BLUE_DARK, 1.4), line(4, 11, 12, 11, BLUE_DARK, 1.4)),
    'edit-select-none': svg(rect(1.5, 1.5, 13, 13, 'none', GRAY, 0, 1.2).replace('/>', ' stroke-dasharray="2 1.5"/>')),
    'edit-select-invert': svg(rect(1.5, 1.5, 13, 13, 'none', BLUE, 0, 1.2).replace('/>', ' stroke-dasharray="2 1.5"/>'),
                              path('M1.5,1.5 h13 l-13,13 z', BLUE_LIGHT)),
    'edit-clear-history': svg(broom()),
    'insert-link': svg(link_chain()),
    'list-add': svg(path('M6.5,1.5 h3 v5 h5 v3 h-5 v5 h-3 v-5 h-5 v-3 h5 z', GREEN, GREEN_DARK)),
    'list-remove': svg(rect(1.5, 6.5, 13, 3, RED, RED_DARK)),
    # Navigation
    'go-previous': svg(arrow('left', 8, 8, 13, BLUE, BLUE_DARK)),
    'go-next': svg(arrow('right', 8, 8, 13, BLUE, BLUE_DARK)),
    'go-up': svg(folder(), arrow('up', 8, 9, 8, GREEN, GREEN_DARK)),
    'go-down': svg(arrow('down', 8, 8, 13, BLUE, BLUE_DARK)),
    'go-top': svg(folder(), rect(5, 7.5, 6, 1.5, GREEN_DARK), arrow('up', 8, 11, 5.5, GREEN, GREEN_DARK)),
    'go-bottom': svg(arrow('down', 8, 7, 10, BLUE, BLUE_DARK), rect(3, 13, 10, 1.5, BLUE_DARK)),
    'go-jump': svg(path('M2.5,12.5 c1,-6 5,-8 9,-8', 'none', BLUE_DARK, 3.2), path('M2.5,12.5 c1,-6 5,-8 9,-8', 'none', BLUE, 1.8),
                   '<polygon points="10,1.5 14.5,4.5 10,7.5" fill="%s" stroke="%s" stroke-width="0.8"/>' % (BLUE, BLUE_DARK)),
    'go-down-search': svg(arrow('down', 6, 7, 10, BLUE, BLUE_DARK), group('translate(7,7) scale(0.55)', magnifier())),
    'view-refresh': svg(refresh_arrows()),
    'view-filter': svg(funnel()),
    'view-list-details': svg(queue_list()),
    'view-list-tree': svg(tree_nodes()),
    'view-split-left-right': svg(panes()),
    'view-sort': svg(line(3, 3, 13, 3, BLUE_DARK, 1.6), line(3, 6.5, 11, 6.5, BLUE_DARK, 1.6), line(3, 10, 8, 10, BLUE_DARK, 1.6),
                     line(3, 13.5, 5.5, 13.5, BLUE_DARK, 1.6)),
    'view-sort-ascending': svg(sort_az(True)),
    'sort-name': svg(sort_az(True)),
    'view-calendar-day': svg(calendar()),
    'window-duplicate': svg(duplicate_windows()),
    # Transfers
    'cloud-upload': svg(page(6.5, 0.5, 8, 10, False), page(1.5, 4.5, 8, 11),
                        arrow(45, 11, 11, 8, BLUE, BLUE_DARK)),
    'download': svg(page(6.5, 0.5, 8, 10, False), page(1.5, 4.5, 8, 11),
                    arrow(225, 10.5, 11.5, 8, GREEN, GREEN_DARK)),
    # Sessions
    'network-connect': svg(plug(True)),
    'network-disconnect': svg(plug(False)),
    'tab-new': svg(tab_shape(), badge('plus', 11.5, 11.5, 3.2)),
    'tab-close': svg(tab_shape(), badge('x', 11.5, 11.5, 3.2)),
    'tab-duplicate': svg(group('translate(2,-2)', tab_shape(GRAY_LIGHT)), tab_shape()),
    'utilities-terminal': svg(terminal()),
    'system-run': svg(gear(6.5, 6.5, 5), arrow('right', 12, 12, 6, GREEN, GREEN_DARK)),
    'system-shutdown': svg(power()),
    'application-exit': svg(door_exit()),
    'configure': svg(gear()),
    'color-management': svg(palette()),
    # The queue
    'media-playback-start': svg('<polygon points="4,2 13.5,8 4,14" fill="%s" stroke="%s" stroke-width="1" stroke-linejoin="round"/>'
                                % (GREEN, GREEN_DARK)),
    'media-playback-pause': svg(rect(3.5, 2.5, 3.5, 11, BLUE, BLUE_DARK, 0.5), rect(9, 2.5, 3.5, 11, BLUE, BLUE_DARK, 0.5)),
    'process-stop': svg(path('M5.5,1.5 h5 l4,4 v5 l-4,4 h-5 l-4,-4 v-5 z', RED, RED_DARK),
                        line(5.5, 5.5, 10.5, 10.5, '#ffffff', 1.8), line(10.5, 5.5, 5.5, 10.5, '#ffffff', 1.8)),
    # Messages
    'dialog-question': svg(circle_sign(BLUE, BLUE_DARK, '?')),
    'dialog-error': svg(circle_sign(RED, RED_DARK, '!')),
    'dialog-password': svg(key()),
    'dialog-ok': svg(badge('check', 8, 8, 6.5)),
    'help-about': svg(circle_sign(BLUE, BLUE_DARK, 'i')),
    'help-contents': svg(book()),
    # People and security
    'user-identity': svg(person()),
    'system-users': svg(group('translate(3,0) scale(0.8)', person(8, 1.0, GREEN, GREEN_DARK)),
                        group('translate(-1,3) scale(0.8)', person())),
    'object-locked': svg(padlock()),
    'object-unlocked': svg(padlock(True)),
}

# The dialog images ----------------------------------------------------------
# The 32-pixel pictures of upstream's dialogs (its DialogImages), named as
# upstream's (LoadDialogImage), lowercase with dashes; they are not a part of
# the theme, a dialog shows them whatever the icons are.

def svg32(*parts):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">\n' +
            '\n'.join(parts) + '\n</svg>\n')


def page32(x, y, w=16, h=21, faded=False):
    fold = 5
    stroke, fold_fill, lines = (PAGE_FADED, '#eef1f4', '#dde2e8') if faded else (PAGE_STROKE, PAGE_FOLD, PAGE_LINE)
    s = path('M%g,%g h%g l%g,%g v%g h%g z' % (x, y, w - fold, fold, fold, h - fold, -w), PAGE, stroke)
    s += path('M%g,%g v%g h%g' % (x + w - fold, y, fold, fold), fold_fill, stroke)
    ly = y + 8
    while ly < y + h - 2.5:
        s += line(x + 3, ly, x + w - 3, ly, lines, 1.2)
        ly += 3
    return s


def folder32(x, y, w, h, open_=False):
    tab = w * 0.36
    back = path('M%g,%g h%g l%g,%g h%g v%g h%g z' % (x, y, tab, h * 0.18, h * 0.18, w - tab - h * 0.18, h, -w),
                FOLDER_BACK, FOLDER_STROKE)
    top = y + h * 0.32
    if open_:
        front = path('M%g,%g h%g l%g,%g h%g z' % (x + w * 0.16, top + h * 0.08, w * 0.98, -w * 0.16, h * 0.92 - h * 0.08,
                                                    -w * 0.98), FOLDER_FRONT, FOLDER_STROKE)
    else:
        front = rect(x, top, w, h + y - top, FOLDER_FRONT, FOLDER_STROKE)
    return back + front


def curved_arrow32(d, end, direction, color, dark):
    # A thick curve with its head at the end
    return (path(d, 'none', dark, 5) + path(d, 'none', color, 3) +
            arrow_head(end[0], end[1], direction[0], direction[1], 4, color, dark))


def transfer_pages(faded_source, source_back):
    # A page copied from the back one to the front one, or the other way
    return (page32(12.5, 1.5, faded=faded_source and source_back) +
            page32(3.5, 8.5, faded=faded_source and not source_back))


def side_pages(faded_source):
    return page32(2.5, 4.5, 15, 20, faded_source) + page32(14.5, 1.5, 15, 20)


def circle_sign32(fill, stroke, sign):
    s = circle(16, 16, 13.5, fill, stroke, 1.2)
    if sign == 'i':
        s += circle(16, 9.3, 2.3, '#ffffff') + rect(13.9, 13.4, 4.2, 11.2, '#ffffff', None, 1.2)
    elif sign == '?':
        s += path('M11.4,12.2 a4.8,4.8 0 1 1 7.2,4.1 c-1.7,1 -2.6,1.9 -2.6,3.9', 'none', '#ffffff', 3.6)
        s += circle(16, 24.6, 2.2, '#ffffff')
    elif sign == 'x':
        s += line(10.5, 10.5, 21.5, 21.5, '#ffffff', 3.6) + line(21.5, 10.5, 10.5, 21.5, '#ffffff', 3.6)
    return s


def window32(fill='#ffffff', title=BLUE, title_dark=BLUE_DARK, frame=GRAY_DARK):
    return rect(2.5, 4.5, 27, 23, fill, frame, 1, 1.2) + rect(2.5, 4.5, 27, 4.5, title, title_dark, 1)


TILE_COLORS = (FOLDER_FRONT, GREEN, BLUE, RED, PURPLE, ORANGE)


def commander32():
    s = window32() + line(16, 9, 16, 27.5, GRAY_DARK)
    for i, y in enumerate((12.5, 16, 19.5, 23)):
        for x in (4.5, 18):
            s += rect(x, y - 1.2, 2.4, 2.4, TILE_COLORS[(i + int(x)) % len(TILE_COLORS)])
            s += line(x + 4, y, x + 9.5, y, PAGE_LINE, 1.4)
    return s


def explorer32():
    s = window32() + line(10, 9, 10, 27.5, GRAY_DARK)
    for y in (12.5, 16, 19.5):
        s += line(4.5, y, 8, y, PAGE_LINE, 1.4)
    for row, y in enumerate((11.5, 18.5)):
        for col, x in enumerate((12.5, 18.5, 24.5)):
            s += rect(x, y, 4, 4, TILE_COLORS[(row * 3 + col) % len(TILE_COLORS)], None, 0.6)
            s += line(x, y + 6, x + 4, y + 6, PAGE_LINE, 1)
    return s


def console32():
    return (rect(2.5, 4.5, 27, 23, SCREEN, '#111820', 1.2, 1.2) + rect(2.5, 4.5, 27, 4.5, GRAY, GRAY_DARK, 1) +
            path('M7.5,14 l4.5,3.8 l-4.5,3.8', 'none', '#7CFC72', 2.2) + line(14.5, 22.5, 20.5, 22.5, '#7CFC72', 2.2))


DIALOG_IMAGES = {
    'download-file': svg32(transfer_pages(False, True), arrow(225, 20.5, 21.5, 17, GREEN, GREEN_DARK, 1.2)),
    'download-file-remove-original': svg32(transfer_pages(True, True),
                                           arrow(225, 20.5, 21.5, 17, GREEN, GREEN_DARK, 1.2)),
    'upload-file': svg32(transfer_pages(False, False), arrow(45, 22.5, 20, 17, BLUE, BLUE_DARK, 1.2)),
    'upload-file-remove-original': svg32(transfer_pages(True, False), arrow(45, 22.5, 20, 17, BLUE, BLUE_DARK, 1.2)),
    'duplicate-l-to-r': svg32(side_pages(False), arrow(90, 22.5, 22.5, 17, AMBER, AMBER_DARK, 1.2)),
    'move-l-to-r': svg32(side_pages(True), arrow(90, 22.5, 22.5, 17, AMBER, AMBER_DARK, 1.2)),
    'multiple-files': svg32(page32(12.5, 1.5, 16, 20), page32(8, 5.5, 16, 20), page32(3.5, 9.5, 16, 20)),
    'delete-file': svg32(line(7.5, 7.5, 24.5, 24.5, RED_DARK, 7.4), line(24.5, 7.5, 7.5, 24.5, RED_DARK, 7.4),
                         line(7.5, 7.5, 24.5, 24.5, RED, 5), line(24.5, 7.5, 7.5, 24.5, RED, 5)),
    'open-folder': svg32(folder32(2.5, 12.5, 25, 16, True),
                         curved_arrow32('M8.5,11.5 C10,3.5 20,1.5 25,7', (25, 7), (5, 5.5), BLUE, BLUE_DARK)),
    'synchronize-directories': svg32(folder32(2.5, 5.5, 27, 22),
                                     group('translate(5,7.5) scale(1.38)', refresh_arrows(8, 8, 5, GREEN, GREEN_DARK))),
    'keep-remote-directory-up-to-date': svg32(
        folder32(1.5, 2.5, 13, 10), folder32(17.5, 19.5, 13, 10),
        curved_arrow32('M17,5.5 C22.5,4 26.5,7.5 26.5,13.5', (26.5, 13.5), (0, 1), GREEN, GREEN_DARK),
        curved_arrow32('M15,26.5 C9.5,28 5.5,24.5 5.5,18.5', (5.5, 18.5), (0, -1), ORANGE, ORANGE_DARK)),
    'open-console-window': svg32(console32()),
    'commander': svg32(commander32()),
    'explorer': svg32(explorer32()),
    'error': svg32(circle_sign32(RED, RED_DARK, 'x')),
    'help-blue': svg32(circle_sign32(BLUE, BLUE_DARK, '?')),
    'information': svg32(circle_sign32(BLUE, BLUE_DARK, 'i')),
    'warning': svg32(path('M16,2.5 L30,27.5 H2 Z', YELLOW, YELLOW_DARK, 1.4),
                     rect(14.4, 10, 3.2, 10, '#4a3b00', None, 1.2), circle(16, 23.4, 1.9, '#4a3b00')),
}

INDEX_THEME = """[Icon Theme]
Name=LinSCP
Comment=The icons of LinSCP
Inherits=hicolor
Directories=scalable

[scalable]
Size=16
MinSize=8
MaxSize=512
Type=Scalable
"""


def write(output):
    if output.exists():
        shutil.rmtree(output)
    (output / 'scalable').mkdir(parents=True)
    (output / 'index.theme').write_text(INDEX_THEME)
    for name, content in sorted(ICONS.items()):
        (output / 'scalable' / (name + '.svg')).write_text(content)
    (output / 'dialogs').mkdir()
    for name, content in sorted(DIALOG_IMAGES.items()):
        (output / 'dialogs' / (name + '.svg')).write_text(content)
    (output / 'linscp.svg').write_text(app_icon())


def sheet(png):
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        files = []
        for name, content in sorted(ICONS.items()):
            source = tmp / (name + '.svg')
            source.write_text(content)
            for size in (16, 32):
                target = tmp / ('%s-%d.png' % (name, size))
                subprocess.run(['rsvg-convert', '-w', str(size), '-h', str(size), '-o', str(target), str(source)],
                               check=True)
            files.append(str(tmp / (name + '-32.png')))
        for name, content in sorted(DIALOG_IMAGES.items()):
            source = tmp / ('dialog-%s.svg' % name)
            source.write_text(content)
            target = tmp / ('dialog-%s.png' % name)
            subprocess.run(['rsvg-convert', '-w', '32', '-h', '32', '-o', str(target), str(source)], check=True)
            files.append(str(target))
        app = tmp / 'linscp.svg'
        app.write_text(app_icon())
        subprocess.run(['rsvg-convert', '-w', '64', '-h', '64', '-o', str(tmp / 'app.png'), str(app)], check=True)
        files.append(str(tmp / 'app.png'))
        subprocess.run(['montage', *files, '-tile', '12x', '-geometry', '40x40+4+4', '-background', '#e8e8e8', png],
                       check=True)


def main():
    if len(sys.argv) > 2 and sys.argv[1] == '--sheet':
        sheet(sys.argv[2])
        return
    root = Path(__file__).resolve().parents[2]
    output = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'source' / 'qt' / 'icons' / 'linscp'
    write(output)


if __name__ == '__main__':
    main()
