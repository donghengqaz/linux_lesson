#!/usr/bin/python3.5

import sys, platform, time, random, serial, socket
from PyQt5 import QtCore, QtGui, QtWidgets, Qt

import board

class board(board.Ui_MainWindow):
    def __init__(self, port=14212, using_str=False):
        super(board, self).__init__()

        app = QtWidgets.QApplication(sys.argv)
        self.main_board = QtWidgets.QMainWindow()

        self.setupUi(self.main_board)

        self.port = port
        self.using_str = using_str

        self.board_init()

        self.main_board.show()
        sys.exit(app.exec_())

    def event_msg(self, id, val):
        data = Qt.QByteArray()

        s_val = "%04d" % val
        data.append(s_val)

        s_id = "%04d" % id
        data.append(s_id)

        return data

    def button_event(self, val):
        self.udp.writeDatagram(self.event_msg(val, 1), self.remote_addr, self.remote_port)

    def button_cb1(self):
        self.button_event(5)

    def button_cb2(self):
        self.button_event(6)

    def button_cb3(self):
        self.button_event(7)

    def button_cb4(self):
        self.button_event(8)

    def board_init(self):
        self.udp = Qt.QUdpSocket()
        self.udp.bind(Qt.QHostAddress("0.0.0.0"), self.port)
        self.udp.readyRead.connect(self.recv_udp)
        self.udp.joinMulticastGroup(Qt.QHostAddress("224.0.2.66"))
        self.udp.setSocketOption(3, 0)

        self.pushButton1.pressed.connect(self.button_cb1)
        self.pushButton2.pressed.connect(self.button_cb2)
        self.pushButton3.pressed.connect(self.button_cb3)
        self.pushButton4.pressed.connect(self.button_cb4)

        self.remote_addr = Qt.QHostAddress("224.0.2.66")
        self.remote_port = 14212

    def get_obj(self, b, s, l):
        v = 0
        for i in b[s:s+l]:
            v += i
        return v

    def recv_udp(self):
        event = self.udp.readDatagram(12)[0]
        if len(event) < 12:
            return

        _id  = self.get_obj(event, 0, 4)
        _num = self.get_obj(event, 4, 4)
        _val = self.get_obj(event, 8, 4)

        self.event_handle(_id, _num, _val)

    def set_led(self, num, state):
        num_tup = (self.textLed1, self.textLed2, self.textLed3, self.textLed4)
        state_tup = ("background-color:white", "background-color:green")
        num_tup[num].setStyleSheet(state_tup[state])

    def event_handle(self, id, num, val):
        event_cb_tup = ( "NULL",
                         self.set_led)

        print(id, num, val)
        event_cb_tup[id](num, val)

if __name__ == "__main__":
    main_board = board()
