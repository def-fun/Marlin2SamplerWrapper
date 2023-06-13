立创EDA专业版 项目文件和元件库文件

-------

# 供电

## 防反接

自恢复保险丝+二极管

## 防过压

+ 气体放电管
+ TVS二极管 SMBJ6.5CA等
+ 宽电压支持

## 隔离

+ 有个外壳
+ 大功率时要有散热
+ 电路板涂胶防水

## 降压

+ 芯片
  LM2576
  MP1584EN
+ 模块
  金升阳DC-DC电源模块，如WRB0503S-1WR2，4.5~
  9v输入，3.3v输出，功率1W，[WRB0503S-1WR2_规格书](/doc/电子元件手册/电源/C5832159_电源模块_WRB0503S-1WR2_规格书_MORNSUN(金升阳)电源模块规格书.PDF)

## 升压

## 稳压

LDO三端线性稳压 AMS1117

# 通信

## TTL 3.3V和5V电平兼容

[3.3V与5V系统电平兼容的方法探究](https://blog.csdn.net/RF_star/article/details/105838726)

MOS管 https://blog.csdn.net/qq_44711012/article/details/121843559

## RS485-TTL转换

用MAX13487可以替代MAX485，自动改变串口数据流向

MAX485+三极管，可以自动改变串口数据流向

## 信号隔离、电磁兼容

电源隔离和信号隔离都有才能起到隔离作用

信号隔离可以使用光耦隔离（需要考虑开关频率和通信带宽的匹配），也可以用专用隔离芯片

常用数字隔离器芯片有：ADUM1201、ADUM5241、ISO7221和Si8421等
注意：设计串口隔离电路时，不仅串口通信线TX和RX需要隔离，两侧的电源和地也需要隔离，否则隔离就没有意义了。
ADUM1201：
● 隔离器的技术构架：Magnetic Coupling
●类型：General Purpose
●通道数：2
●通道类型：Unidirectional
●隔离电压：2500Vrms
●数据速率：10Mbps
ADUM5241：
●隔离器的技术构架：Magnetic Coupling
●类型：General Purpose
●通道数：2
●通道类型：Unidirectional
●隔离电压：2500Vrms
●数据速率：1Mbps
ISO7221：
●隔离器的技术构架：Capacitive Coupling
●类型：General Purpose
●通道数：2
●通道类型：Unidirectional
●隔离电压：2500Vrms
●数据速率：150Mbps
Si8421：
●隔离器的技术构架：Capacitive Coupling
●类型：General Purpose
●通道数：2
●通道类型：Unidirectional
●隔离电压：2500Vrms
●数据速率：150Mbps
还有隔离电压比较高的，对隔离电压要求高的可以考虑一下这个系列的芯片
π121M31：
●隔离器的技术构架：iDivider
●通道数：2
●隔离电压：3000Vrms
●数据速率：10Mbps
π122U31：
●隔离器的技术构架：iDivider
●通道数：2
●隔离电压：3000Vrms
●数据速率：150kbps
————————————————
版权声明：本文为CSDN博主「永不停歇的水手」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/weixin_42906535/article/details/123869661

USB隔离只能用ADuM3160

PCB上，Tx/Rx要有指示灯，MAX485等芯片的控制角可选单片机控制和三极管控制

## 3.3~5.0V TTL 宽电压支持

+ 方案1.

+ 方案2. 电平转换

TXS0101 1通道全双工双向电平转换
SN74AVC1T45

+ 方案3. 使用支持宽电压TTL的串口芯片
