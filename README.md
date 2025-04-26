# IOCP Echo Server

## 📌 목차
- [개요](#개요)
- [프로젝트 설명](#프로젝트-설명)
- [주요 기능 설명](#주요기능_설명)
- [테스트](#테스트)
- [폴더 구조](#폴더-구조)

---

## 개요

- **프로젝트 이름**: IOCP Echo Server
- **개발 기간**: 2025.02 ~ 2025.03
- **개발 언어**: C++
- **개발 인원**: 1명

---

## 프로젝트 설명

IOCP 기반의 Echo 서버입니다.

`CLanServer` 추상 클래스를 상속받아 `CEchoServer`를 구현하였습니다.

서버는 `Start()` 호출 시 Winsock 초기화와 listen 소켓 설정을 수행합니다.

`AcceptThread`, `WorkerThread`, `ReleaseThread`를 생성하여  
클라이언트 수락, 네트워크 I/O 처리, 세션 해제를 각각 담당합니다.

메시지 수신 시 `SerializePacket`을 파싱하여  
동일한 데이터를 클라이언트로 다시 전송하는 에코 로직을 `OnMessage()`에서 수행합니다.

---

## 테스트

### 용어 설명
- **OverSend**: IOCP 워커 스레드가 Send 완료 통지를 받기 전에도 동시에 발송할 수 있는 WSASend 요청의 최대 개수(윈도우 크기).  
- **Disconnect Delay**: 더미 클라이언트가 연결을 끊은 후 재연결을 시도하기 전까지 대기하는 시간(ms).  
- **Loop Delay**: 전체 에코 루프(클라이언트별 Echo 요청 검사 및 Send 호출) 사이의 대기 시간(ms).

#### 실행화면 주요 항목
- `Error – Connect Fail`: 클라이언트 `connect()` 실패 횟수  
- `Error – Disconnect from Server`: 서버가 클라이언트 연결을 일방적으로 끊은 횟수  
  (정상: `AcceptTotal` == 더미의 `ConnectTotal`)  
- `Error – LoginPacket …`: 무시  
- `Error – Echo Not Recv`: 500ms 이상 응답 없는 경우 (테스트 중 절대 누적되지 않아야 함)  
- `Error – Packet Error`: 전송·수신 값 불일치 (절대 발생하면 안 됨)  

> `*` 표시된 에러 항목은 테스트 중 발생해서는 안 됩니다.

### 추가 스트레스 테스트 시나리오
1. **연결/해제 반복**  
   - 설정된 `Client Count` 만큼 접속 → `Disconnect Test=Yes` 상태에서 랜덤하게 서버 연결 해제 및 재접속
     

2. **데이터 무결성 검증**  
   - 각 클라이언트 세션마다 8바이트 고유 증가 값 전송  
   - 서버로부터 받은 값을 자신이 보낸 값과 **1:1 일치**하는지 비교  
3. **Overlapped I/O 정확성**  
   - `Wait Echo Count` (또는 내부 `m_IOCount`)가 I/O 요청 전후로 정확히 증가·감소하여  
     Overlapped I/O 종료 절차가 올바르게 처리되는지 확인  
4. **검증 항목**  
   - 서버의 **접속 거부**(reject) 발생 여부  
   - 서버에서 **강제 연결 해제** 발생 여부  
   - 전송 값과 수신 값의 **일치** 여부  


---

## 폴더 구조
<PRE>
📦 IOCP-Echo-Server (ver2)
┣ 📂Core               # 공통 헤더 및 유틸
┃ ┣ 📜 pch.h
┃ ┣ 📜 Define.h
┃ ┣ 📜 RingBuffer.h
┃ ┗ 📜 SerializePacket.h
┣ 📂Network            # IOCP 기반 네트워크 서버
┃ ┣ 📜 CLanServer.h
┃ ┣ 📜 CLanServer.cpp
┃ ┣ 📜 CEchoServer.h
┃ ┗ 📜 CEchoServer.cpp
┣ 📂Session            # 세션 관리
┃ ┗ 📜 Session.h
┣ 📂Main               # 진입점
┃ ┣ 📜 Main.h
┃ ┗ 📜 Main.cpp
┗ 📂Utils              # 로깅 등 공통 모듈
  ┣ 📜 SystemLog.h
  ┗ 📜 SystemLog.cpp
</PRE>
