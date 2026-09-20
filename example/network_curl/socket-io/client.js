// === 타임스탬프 로깅 재정의 ===
const originalLog = console.log;
const originalError = console.error;

const getTimestamp = () => {
  const now = new Date();
  const pad = (n, s = 2) => String(n).padStart(s, '0');
  return `[${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())} ` +
         `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}.${pad(now.getMilliseconds(), 3)}]`;
};

console.log = (...args) => originalLog(getTimestamp(), ...args);
console.error = (...args) => originalError(getTimestamp(), ...args);
// =============================

const ioV2 = require('socket.io-client-v2');
const ioV3 = require('socket.io-client-v3');
const ioV4 = require('socket.io-client-v4');

// ==========================================
// [v2] Port 52000 서버 연결
// ==========================================
const socketV2 = ioV2('http://localhost:52000');

socketV2.on('connect', () => {
  console.log(`✅ [Client v2] 연결 성공 (Port 52000, ID: ${socketV2.id})`);
  socketV2.emit('ping_v2', { from: 'Node.js Client v2', time: Date.now() });
});

socketV2.on('pong_v2', (data) => {
  console.log('📩 [Client v2] 응답 수신:', data);
});

socketV2.on('connect_error', (err) => {
  console.error('❌ [Client v2] 연결 실패:', err.message);
});

// ==========================================
// [v3] Port 53000 서버 연결
// ==========================================
const socketV3 = ioV3('http://localhost:53000');

socketV3.on('connect', () => {
  console.log(`✅ [Client v3] 연결 성공 (Port 53000, ID: ${socketV3.id})`);
  socketV3.emit('ping_v3', { from: 'Node.js Client v3', time: Date.now() });
});

socketV3.on('pong_v3', (data) => {
  console.log('📩 [Client v3] 응답 수신:', data);
});

socketV3.on('connect_error', (err) => {
  console.error('❌ [Client v3] 연결 실패:', err.message);
});

// ==========================================
// [v4] Port 54000 서버 연결
// ==========================================
const socketV4 = ioV4('http://localhost:54000');

socketV4.on('connect', () => {
  console.log(`✅ [Client v4] 연결 성공 (Port 54000, ID: ${socketV4.id})`);
  socketV4.emit('ping_v4', { from: 'Node.js Client v4', time: Date.now() });
});

socketV4.on('pong_v4', (data) => {
  console.log('📩 [Client v4] 응답 수신:', data);
});

socketV4.on('connect_error', (err) => {
  console.error('❌ [Client v4] 연결 실패:', err.message);
});