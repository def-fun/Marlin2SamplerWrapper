#!/usr/bin/env python3
"""
计算、检查modbus数据是否正确
"""


def is_modbus(string: str) -> bool:
    """
    判断一组十六进制数据是否满足modus格式
    :param string:
    :return:
    """
    string = string.replace(' ', '')
    payload = string[:-4]
    end_int = int('0x' + string[-4:], 16)
    crc16_int = calc_crc16(payload)
    # print(payload)
    # print(end_int)
    # print(crc16_int)
    return end_int == crc16_int


def calc_crc16(string: str) -> int:
    """
    计算crc16
    :param string: 如'00 11'，表示0x00和0x11
    :return: int
    """
    data = bytearray.fromhex(string)
    # logging.info(type(data))
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for i in range(8):
            if (crc & 1) != 0:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1

    return ((crc & 0xff) << 8) + (crc >> 8)


def fill_crc():
    while 1:
        i = input('[i]: ')
        crc_int = calc_crc16(i)
        crc_str = hex(crc_int).replace('0x', '').zfill(4)
        print(i, crc_str[:2], crc_str[2:])


if __name__ == "__main__":
    print(hex(calc_crc16('14 06 00 01 00 01')))  # 0x1b0f
    print(is_modbus('1406000100011b0f'))  # True
    print(is_modbus('09 06 00 0a 00 14 a8 8f'))  # True
    print(is_modbus('09 06 00 0a 00 14'))  # False
