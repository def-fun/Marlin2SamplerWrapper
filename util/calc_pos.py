"""
计算样品瓶坐标，过程见/img/calc_pos.jpg
参考：
    https://blog.csdn.net/key800700/article/details/120252857
    点A(x1,y1)绕某坐标B(x2,y2)顺时针旋转（θ度）后坐标C(x,y)：
    x = (x1 - x2) * cos(θ) + (y1 - y2) * sin(θ) + x2
    y = (y1 - y2) * cos(θ) - (x1 - x2) * sin(θ) + y2
"""
import math

x0 = 0
y0 = 0
xm = 0
yn = 2 ** 0.5

# n、m为间隔数，等于格点数-1
m = 1
n = 1

# 第j个点
j = 3
assert j <= (m + 1) * (n + 1)

coso = ((xm - x0) * m + (yn - y0) * n) / ((xm - x0) ** 2 + (yn - y0) ** 2) ** 0.5 / (m ** 2 + n ** 2) ** 0.5
coso = min(1.0, coso)
print('cos(θ)=', coso)
print('弧度制θ=', math.acos(coso))  # 旋转的角度
print('角度制θ=', float(math.acos(coso) * 180 / math.pi))
a = ((xm - x0) * coso + (yn - y0) * (1 - coso ** 2) ** 0.5) / m
print('边长a=', a)  # 网格的边长

delta_x = int((j - 1) / (n + 1)) * a
delta_y = ((j - 1) % (n + 1)) * a
sino = (1 - coso ** 2) ** 0.5
xj = delta_x * coso - delta_y * sino + x0
yj = delta_x * sino + delta_y * coso + y0
print(f'第{j}个点的坐标为({xj}, {yj})')
