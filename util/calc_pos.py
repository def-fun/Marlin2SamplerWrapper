"""
计算样品瓶坐标
"""
import math

x0 = 0
y0 = 0
xm = 0
yn = 1
m = 1
n = 1

coso = ((xm - x0) * m + (yn - y0) * n) / ((xm - x0) ** 2 + (yn - y0) ** 2) ** 0.5 / (m ** 2 + n ** 2) ** 0.5
coso = min(1.0, coso)
print('cos(θ)=', coso)
print('θ=', math.acos(coso))
a_x = (xm - x0) / (m * coso - n * (1 - coso ** 2) ** 0.5)
a_y = (yn - y0) / (m * (1 - coso ** 2) ** 0.5 + n * coso)
print('a_x=', a_x)
print('a_y=', a_y)
