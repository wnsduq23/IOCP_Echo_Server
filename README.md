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
- **개발 기간**: 2025.02 ~ 2024.03
- **개발 언어**: C++
- **개발 인원**: 1명

---

## 프로젝트 설명

IOCP 기반의 Echo 서버입니다. CLanServer 추상 클래스를 상속받아 CEchoServer를 구현하였습니다. 
서버는 Start() 호출 시 Winsock 초기화와 listen 소켓 설정을 수행하며, 
AcceptThread / WorkerThread / ReleaseThread를 생성하여 클라이언트 수락, 네트워크 I/O 처리, 세션 해제를 각각 담당합니다. 
메시지 수신 시 SerializePacket을 파싱하여 동일한 데이터를 클라이언트로 다시 전송하는 에코 로직은 OnMessage()에서 수행됩니다.

---

## 테스트
(추가 바람)

---

## 폴더 구조

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
