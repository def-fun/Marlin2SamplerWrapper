"""
计算托盘逆时针旋转后的样品瓶的坐标，过程见/img/calc_pos.jpg
参考：
    https://blog.csdn.net/key800700/article/details/120252857
    点A(x1,y1)绕某坐标B(x2,y2)顺时针旋转（θ度）后坐标C(x,y)：
    x = (x1 - x2) * cos(θ) + (y1 - y2) * sin(θ) + x2
    y = (y1 - y2) * cos(θ) - (x1 - x2) * sin(θ) + y2
"""
import math

# 离原点(0, 0)最近的点的坐标
x0 = 0
y0 = 0
# 对角线位置的点的坐标
xm = 100
yn = 100

# n、m为间隔数，等于格点数-1。比如瓶子有5行10列，则n=4, m=9，n<=m
n = 4
m = 9

assert n <= m
coso = ((xm - x0) * m + (yn - y0) * n) / ((xm - x0) ** 2 + (yn - y0) ** 2) ** 0.5 / (m ** 2 + n ** 2) ** 0.5
coso = min(1.0, coso)
print('cos(θ)=', coso)
print('弧度制θ=', math.acos(coso))  # 旋转的角度
print('角度制θ=', float(math.acos(coso) * 180 / math.pi))
a = ((xm - x0) * coso + (yn - y0) * (1 - coso ** 2) ** 0.5) / m
print('边长a=', a)  # 网格的边长


def calc_pos(p: int):
    # 计算第p个点的坐标
    assert 1 <= p <= (m + 1) * (n + 1)
    delta_x = int((p - 1) / (n + 1)) * a
    delta_y = ((p - 1) % (n + 1)) * a
    sino = (1 - coso ** 2) ** 0.5
    xj = delta_x * coso - delta_y * sino + x0
    yj = delta_x * sino + delta_y * coso + y0
    xj = round(xj, 3)
    yj = round(yj, 3)
    # 保留3位小数
    print(f'第{p}个点的坐标为 ({xj}, {yj})')
    return xj, yj


def plot():
    import matplotlib.pyplot as plt

    # 设置等比例坐标轴
    ax = plt.gca()
    ax.set_aspect(1)

    points_x = []
    points_y = []
    for j in range(1, (m + 1) * (n + 1) + 1):
        x, y = calc_pos(j)
        points_x.append(x)
        points_y.append(y)
        # plt.text(x, y, f'P{j}({x}, {y})')  # 序号+坐标标注
        plt.text(x, y, f'P{j}')  # 序号标注
    plt.plot(points_x, points_y)  # 折线
    plt.scatter(points_x, points_y)  # 点

    # x轴标签
    plt.xlabel('X')
    # y轴标签
    plt.ylabel('Y')

    # 图表标题
    plt.title('calc_position')

    # 显示图形
    plt.show()


plot()
