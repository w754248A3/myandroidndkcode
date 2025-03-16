const net = require('net');
const crypto = require('crypto');

const CONCURRENCY =12;          // 并发连接数
const TEST_TIMES = 100;           // 单连接测试次数
const TIMEOUT_MS = 5000;        // 单次测试超时
const DATA_SIZE_RANGE = [1, 4096]; // 数据大小范围

class ConnectionTester {
    constructor(host, port, connId) {
        this.connId = connId;
        this.host = host;
        this.port = port;
        this.socket = null;
        this.stats = {
            success: 0,
            failed: 0,
            bytesSent: 0
        };
    }

    async connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection({
                host: this.host,
                port: this.port
            }, resolve);
            
            this.socket.on('error', reject);
            this.socket.setNoDelay(true);
        });
    }

    async runTests() {
        try {
            await this.connect();
            console.log(`[连接 ${this.connId}] 已建立`);
            
            for (let i = 1; i <= TEST_TIMES; i++) {
                await this.executeTest(i);
            }
            
            return this.stats;
        } catch (error) {
            console.error(`[连接 ${this.connId}] 错误:`, error.message);
            return { ...this.stats, failed: TEST_TIMES };
        } finally {
            this.socket?.end();
        }
    }

    async executeTest(testNum) {
        const dataSize = this.generateDataSize();
        const testData = crypto.randomBytes(dataSize);
        
        try {
            const received = await this.sendAndWait(testData);
            this.validate(testData, received, testNum);
            this.stats.success++;
            this.stats.bytesSent += dataSize;
        } catch (error) {
            this.stats.failed++;
            console.error(`[连接 ${this.connId}] 测试${testNum}失败:`, error.message);
        }
    }

    async sendAndWait(sendData) {
        return new Promise((resolve, reject) => {
            let buffer = Buffer.alloc(0);
            let timer = null;

            const dataHandler = chunk => {
                buffer = Buffer.concat([buffer, chunk]);
                if (buffer.length === sendData.length) {
                    clearTimeout(timer);
                    this.socket.off('data', dataHandler);
                    resolve(buffer);
                }
            };

            timer = setTimeout(() => {
                this.socket.off('data', dataHandler);
                reject(new Error(`响应超时（${TIMEOUT_MS}ms）`));
            }, TIMEOUT_MS);

            this.socket.on('data', dataHandler);
            this.socket.write(sendData);
        });
    }

    validate(sent, received, testNum) {
        if (!sent.equals(received)) {
            const sentHex = sent.toString('hex').substring(0, 32);
            const recvHex = received.toString('hex').substring(0, 32);
            throw new Error(`数据不匹配\n发送: ${sentHex}...\n接收: ${recvHex}...`);
        }
        console.log(`[连接 ${this.connId}] 测试${testNum}通过 (${sent.length}字节)`);
    }

    generateDataSize() {
        return Math.floor(Math.random() * 
            (DATA_SIZE_RANGE[1] - DATA_SIZE_RANGE[0] + 1)) + DATA_SIZE_RANGE[0];
    }
}

async function runConcurrentTests(host, port) {
    const testers = Array.from({ length: CONCURRENCY }, 
        (_, i) => new ConnectionTester(host, port, i + 1));
    
    const results = await Promise.all(
        testers.map(tester => tester.runTests())
    );

    const summary = results.reduce((acc, curr) => ({
        success: acc.success + curr.success,
        failed: acc.failed + curr.failed,
        bytesSent: acc.bytesSent + curr.bytesSent
    }), { success: 0, failed: 0, bytesSent: 0 });

    console.log(`
===== 测试汇总 =====
总连接数: ${CONCURRENCY}
总测试次数: ${summary.success + summary.failed}
成功: ${summary.success}
失败: ${summary.failed}
数据吞吐量: ${(summary.bytesSent / 1024).toFixed(2)} KB
==================`);
}

// 使用示例
const [host, port] = '192.168.0.105:8086'.split(':');
runConcurrentTests(host, parseInt(port));