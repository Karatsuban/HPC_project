#coding:utf-8

# made by Raphael Garnier

# creates a pictures base on the mandelbrot "bitmap" file out.txt

from PIL import Image, ImageDraw
from math import log


max_iter = 5000

resolution = 2000
WIDTH = 3*resolution
HEIGHT = 2*resolution

im = Image.new("HSV", (WIDTH, HEIGHT), (0,0,0))
draw = ImageDraw.Draw(im)


with open("out.txt", "r") as file:
        lines = file.readlines();
        pos = 0
        for line in lines:
                for pix_val in line.split(","):
                        pix_val = int(pix_val)
                        hue = 255- int(255 * log(pix_val+1) / log(max_iter))
                        #hue = int(255 * pix_val/max_iter)
                        value = 255 if pix_val < max_iter else 0
                        draw.point([pos%WIDTH, pos//WIDTH], (hue, 255, value))
                        pos += 1

im.convert('RGB').save("output.png", 'PNG')
