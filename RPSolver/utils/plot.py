'''
==============
3D scatterplot
==============

Demonstration of a basic scatterplot in 3D.
'''

from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import numpy as np
import csv
from contextlib import ExitStack
import locale
from os import path

def is_valid(row):
    return len(row) > 1

def map_row(row):
    (iteration,
    bigbox_x,
    bigbox_y,
    bigbox_z,
    radiosity_min,
    radiosity_center,
    radiosity_max,
    photons,
    duration,
    time_from_start,
    comment) = row

    return (locale.atof(bigbox_x),
         locale.atof(bigbox_z),
         locale.atof(radiosity_center),
        locale.atof(radiosity_min),
        locale.atof(radiosity_max)
    )

def unzip(lst):
    n = len(lst[0])
    result = [[]] * n
    for idx in range(n):
        result[idx] = [x[idx] for x in lst]
    return tuple(result)

def get_graph():
    locale.setlocale(locale.LC_ALL, 'es')
    csv_path = path.join(path.dirname(__file__), '..', 'examples', 'output', 'solutions.csv')
    with ExitStack() as stack:
        csv_file = stack.enter_context(open(csv_path))
        csv_reader = csv.reader(csv_file, delimiter=';', quotechar='"')
        next(csv_reader)
        rows = [map_row(row)
                 for row in csv_reader
                 if(is_valid(row))]
        return unzip(rows)
    

def make_plot():
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.set_xlabel('X Label')
    ax.set_ylabel('Y Label')
    ax.set_zlabel('Z Label')

    xs, ys, zs, z_min, z_max = get_graph()

    ax.scatter(xs, ys, zs, c='r', marker='o')
    
    for i in range(len(xs)):
        ax.plot([xs[i], xs[i]], [ys[i], ys[i]], [z_min[i], z_max[i]], marker="_", c='b')

    plt.show()

if __name__ == '__main__':
    make_plot()