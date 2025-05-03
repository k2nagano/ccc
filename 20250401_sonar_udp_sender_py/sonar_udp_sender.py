#!/usr/bin/env python3
import time
import cv2
import socket
import struct
import argparse
import sys

def send_chunked(sock, addr, data, max_dgram=60000):
    """データを max_dgram バイトずつに分割して UDP 送信"""
    total = len(data)
    offset = 0
    # オプションでシーケンス番号を先頭に付ける例（バイト長 4）
    seq = 0
    while offset < total:
        chunk = data[offset:offset+max_dgram]
        # シーケンス番号パック例（!I: network-order uint32）
        header = struct.pack('!I', seq)
        sock.sendto(header + chunk, addr)
        offset += max_dgram
        seq += 1

def main():
    parser = argparse.ArgumentParser(description="Send sonar frames over UDP (chunked).")
    parser.add_argument("input_file", help="Path to the MKV file")
    parser.add_argument("--swath", type=int, default=120, help="Swath angle [deg]")
    parser.add_argument("--range", type=int, default=30, help="Sonar range [m]")
    parser.add_argument("--is_16bit", type=int, choices=[0,1], default=0,
                        help="Frame pixel depth flag (1:16bit, 0:8bit)")
    parser.add_argument("--ip", default="127.0.0.1", help="Destination IP")
    parser.add_argument("--port", type=int, default=5700, help="Destination UDP port")
    parser.add_argument("--max", type=int, default=60000,
                        help="Max UDP payload per packet")
    args = parser.parse_args()

    cap = cv2.VideoCapture(args.input_file)
    fps = cap.get(cv2.CAP_PROP_FPS) or 30
    if not cap.isOpened():
        print(f"Error: cannot open {args.input_file}", file=sys.stderr)
        sys.exit(1)

    addr = (args.ip, args.port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            # グレースケール化
            if frame.ndim == 3:
                frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            h, w = frame.shape

            # 型変換
            if args.is_16bit:
                frame = frame.astype('uint16')
            else:
                frame = frame.astype('uint8')

            # ヘッダ（width, height, swath, range, is_16bit）
            hdr = struct.pack('!HHHHI',
                              w, h,
                              args.swath,
                              args.range,
                              args.is_16bit)

            payload = hdr + frame.tobytes()
            # チャンク化送信
            send_chunked(sock, addr, payload, max_dgram=args.max)
            time.sleep(1/fps)

    finally:
        cap.release()
        sock.close()

if __name__ == "__main__":
    main()

