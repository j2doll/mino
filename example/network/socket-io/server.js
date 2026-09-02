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

const express = require('express');
const http = require('http');

const socketIoV2 = require('socket.io-v2');
const { Server: ServerV3 } = require('socket.io-v3');
const { Server: ServerV4 } = require('socket.io-v4');

// 공통 소켓 이벤트 등록 함수 (Ping/Pong 및 Room 처리)
function attachCommonHandlers(socket, versionTag, ioInstance) {
  // 1. 버전별 Ping/Pong
  socket.on(`ping_${versionTag}`, (data) => {
    console.log(`[${versionTag} 수신]`, data);
    socket.emit(`pong_${versionTag}`, { version: versionTag, message: `Hello from Socket.io ${versionTag}!` });
  });

  // 2. Room 입장 처리
  socket.on('join_room', (data) => {
    try {
      const payload = typeof data === 'string' ? JSON.parse(data) : data;
      const room = payload.room;
      socket.join(room);
      console.log(`[${versionTag}] 소켓(${socket.id})이 방 [${room}]에 참여했습니다.`);

      // 해당 방 멤버들에게 입장 환영 브로드캐스트 전송
      ioInstance.to(room).emit('room_broadcast', {
        event: 'user_joined',
        room: room,
        user: socket.id
      });
    } catch (e) {
      console.error(`[${versionTag}] join_room JSON 파싱 에러:`, e);
    }
  });

  // 3. Room 퇴장 처리
  socket.on('leave_room', (data) => {
    try {
      const payload = typeof data === 'string' ? JSON.parse(data) : data;
      socket.leave(payload.room);
      console.log(`[${versionTag}] 소켓(${socket.id})이 방 [${payload.room}]에서 퇴장했습니다.`);
    } catch (e) {}
  });
}

// ==========================================
// [v2] Socket.io v2.x 서버 (Port: 52000)
// ==========================================
const appV2 = express();
const serverV2 = http.createServer(appV2);
const ioV2 = socketIoV2(serverV2, { origins: '*:*' });

ioV2.on('connection', (socket) => {
  console.log(`[v2] 접속: ${socket.id}`);
  attachCommonHandlers(socket, 'v2', ioV2);
  socket.on('disconnect', () => console.log(`[v2] 접속 해제: ${socket.id}`));
});

serverV2.listen(52000, () => {
  console.log('🚀 Socket.io v2 서버가 52000 포트에서 실행 중입니다.');
});

// ==========================================
// [v3] Socket.io v3.x 서버 (Port: 53000)
// ==========================================
const appV3 = express();
const serverV3 = http.createServer(appV3);
const ioV3 = new ServerV3(serverV3, { cors: { origin: '*', methods: ['GET', 'POST'] } });

ioV3.on('connection', (socket) => {
  console.log(`[v3] 접속: ${socket.id}`);
  attachCommonHandlers(socket, 'v3', ioV3);
  socket.on('disconnect', () => console.log(`[v3] 접속 해제: ${socket.id}`));
});

serverV3.listen(53000, () => {
  console.log('🚀 Socket.io v3 서버가 53000 포트에서 실행 중입니다.');
});

// ==========================================
// [v4] Socket.io v4.x 서버 (Port: 54000)
// ==========================================
const appV4 = express();
const serverV4 = http.createServer(appV4);
const ioV4 = new ServerV4(serverV4, { cors: { origin: '*', methods: ['GET', 'POST'] } });

// 1) 기본 루트 네임스페이스 ("/")
ioV4.on('connection', (socket) => {
  console.log(`[v4 루트(/)] 접속: ${socket.id}`);
  attachCommonHandlers(socket, 'v4', ioV4);
  socket.on('disconnect', (reason) => console.log(`[v4] 접속 해제: ${socket.id} (원인: ${reason})`));
});

// 2) 커스텀 네임스페이스 ("/chat") 예시
const chatNamespaceV4 = ioV4.of('/chat');
chatNamespaceV4.on('connection', (socket) => {
  console.log(`[v4 /chat 네임스페이스] 접속: ${socket.id}`);
  attachCommonHandlers(socket, 'v4', chatNamespaceV4);
});

serverV4.listen(54000, () => {
  console.log('🚀 Socket.io v4 서버가 54000 포트에서 실행 중입니다.');
});
