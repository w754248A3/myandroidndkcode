const net = require('net');
const crypto = require('crypto');

const TEST_TIMES = 1000;          // 测试循环次数
const TIMEOUT_MS = 5000;       // 超时时间
const MIN_DATA_SIZE = 1;       // 最小数据字节数
const MAX_DATA_SIZE = 1024;     // 最大数据字节数

async function testNetworkInterface(host, port) {
    const socket = net.createConnection({ host, port });
    socket.setNoDelay(true);    // 禁用Nagle算法

    try {
        // 等待连接建立
        await new Promise((resolve, reject) => {
            socket.once('connect', resolve);
            socket.once('error', reject);
        });

        for (let i = 1; i <= TEST_TIMES; i++) {
            // 生成随机测试数据
            const dataSize = Math.floor(Math.random() * (MAX_DATA_SIZE - MIN_DATA_SIZE + 1)) + MIN_DATA_SIZE;
            const sendData = crypto.randomBytes(dataSize);
            
            // 发送并等待响应
            const receivedData = await new Promise((resolve, reject) => {
                let received = Buffer.alloc(0);
                let timer = null;

                // 接收数据处理器
                const dataHandler = (chunk) => {
                    received = Buffer.concat([received, chunk]);
                    
                    if (received.length === sendData.length) {
                        clearTimeout(timer);
                        socket.off('data', dataHandler);
                        resolve(received);
                    }
                };

                // 设置超时
                timer = setTimeout(() => {
                    socket.off('data', dataHandler);
                    reject(new Error(`测试${i} 超时未收到响应`));
                }, TIMEOUT_MS);

                socket.on('data', dataHandler);
                socket.write(sendData);
            });

            // 校验数据一致性
            if (!sendData.equals(receivedData)) {
                console.error(`❌ 测试${i} 失败：接收数据与发送数据不一致`);
                console.log(`发送数据：${sendData.toString('hex')}`);
                console.log(`接收数据：${receivedData.toString('hex')}`);
                return false;
            }
            console.log(`✅ 测试${i} 通过 (${dataSize}字节)`);
        }
        return true;
    } catch (error) {
        console.error(`❌ 测试失败：`, error.message);
        return false;
    } finally {
        socket.end(); // 关闭连接
    }
}

// 使用示例
const [host, port] = '192.168.0.105:8081'.split(':');
testNetworkInterface(host, parseInt(port))
    .then(success => console.log(success ? '\n所有测试通过！' : '\n测试未通过！'));