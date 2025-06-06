#!/usr/bin/env python3
import subprocess
import threading
import time
from flask import Flask, render_template, request, jsonify
import queue
import os

app = Flask(__name__)

class FilesystemSimulator:
    def __init__(self):
        self.process = None
        self.input_queue = queue.Queue()
        self.output_queue = queue.Queue()
        self.running = False
        
    def start(self):
        if self.running:
            return
            
        try:
            self.process = subprocess.Popen(
                ['./simple_filesystem'],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                universal_newlines=True
            )
            self.running = True
            
            # 启动输出读取线程
            self.output_thread = threading.Thread(target=self._read_output)
            self.output_thread.daemon = True
            self.output_thread.start()
            
            # 启动输入处理线程
            self.input_thread = threading.Thread(target=self._process_input)
            self.input_thread.daemon = True
            self.input_thread.start()
            
        except Exception as e:
            print(f"启动文件系统失败: {e}")
            self.running = False
    
    def _read_output(self):
        while self.running and self.process:
            try:
                line = self.process.stdout.readline()
                if line:
                    self.output_queue.put(line.rstrip())
                elif self.process.poll() is not None:
                    break
            except:
                break
    
    def _process_input(self):
        while self.running and self.process:
            try:
                command = self.input_queue.get(timeout=1)
                if command:
                    self.process.stdin.write(command + '\n')
                    self.process.stdin.flush()
            except queue.Empty:
                continue
            except:
                break
    
    def send_command(self, command):
        if self.running:
            self.input_queue.put(command)
    
    def get_output(self):
        output = []
        try:
            while True:
                line = self.output_queue.get_nowait()
                output.append(line)
        except queue.Empty:
            pass
        return output
    
    def stop(self):
        self.running = False
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=5)
            except:
                self.process.kill()

# 全局文件系统实例
filesystem = FilesystemSimulator()

@app.route('/')
def index():
    return render_template('filesystem.html')

@app.route('/start', methods=['POST'])
def start_filesystem():
    filesystem.start()
    time.sleep(1)  # 等待初始化输出
    output = filesystem.get_output()
    return jsonify({'status': 'started', 'output': output})

@app.route('/command', methods=['POST'])
def send_command():
    command = request.json.get('command', '')
    filesystem.send_command(command)
    
    # 等待一小段时间获取输出
    time.sleep(0.5)
    output = filesystem.get_output()
    
    return jsonify({'output': output})

@app.route('/output', methods=['GET'])
def get_output():
    output = filesystem.get_output()
    return jsonify({'output': output})

@app.route('/stop', methods=['POST'])
def stop_filesystem():
    filesystem.stop()
    return jsonify({'status': 'stopped'})

if __name__ == '__main__':
    # 确保可执行文件存在
    if not os.path.exists('./simple_filesystem'):
        print("错误: simple_filesystem 可执行文件不存在")
        print("请先运行: make -f Makefile_simple")
        exit(1)
    
    app.run(host='0.0.0.0', port=12000, debug=True)