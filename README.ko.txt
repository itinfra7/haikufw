haikufw 매뉴얼 (한국어)

개요

haikufw 는 Haiku용 실제 inbound 방화벽이다. 사용자 공간 서버가 패킷을
받기 전에, 커널/네트워크 스택 경로에서 규칙을 적용한다.

현재 구현 방식은 다음과 같다.
- stock TCP/UDP 프로토콜 모듈은 그대로 유지한다
- Haiku 의 IPv4/IPv6 receive 경로를 패치한다
- 패치된 `ipv4`/`ipv6` 프로토콜 모듈이 `haikufw_core`에 질의한다
- 라이브 제어는 `/dev/misc/haikufw`를 통해 수행한다
- 텍스트 규칙 파싱 및 로드는 `haikufwctl`이 담당한다
- 부팅 시 `haikufw-boot.sh`가 설정 파일을 다시 로드한다

이 배포물에 포함된 것

- 커널 규칙 엔진:
  - `kernel/core/`
- control driver:
  - `kernel/control_driver/`
- receive-path hook 패치:
  - `patches/ipv4-receive-hook.patch`
  - `patches/ipv6-receive-hook.patch`
- 빌드에 필요한 vendored Haiku IPv4/IPv6 protocol source:
  - `vendor/haiku/src/add-ons/kernel/network/protocols/ipv4/`
  - `vendor/haiku/src/add-ons/kernel/network/protocols/ipv6/`
- userland 제어 도구:
  - `userland/haikufwctl/`
- 빌드/설치/테스트 스크립트:
  - `scripts/`
- 예시 설정:
  - `config/`

요구 사항

- Haiku x86_64
- 대상 시스템에서 빌드 가능한 툴체인
- `/boot/system/non-packaged` 아래에 파일을 설치할 수 있는 권한

release 패키지에는 patched module 빌드에 필요한 Haiku IPv4/IPv6 protocol
source subset 이 이미 포함되어 있다. 표준 release 설치 절차에서는 별도의
전체 Haiku 소스 checkout 이 필요 없다.

중요한 동작 특성

- 현재 범위는 inbound 필터링이다
- 규칙은 first-match-wins 이다
- 어떤 규칙에도 매치되지 않으면 `default_inbound`가 적용된다
- `reload`는 live 적용이며 재설치가 필요 없다
- install / uninstall 은 patched kernel protocol module 교체가 필요하므로
  reboot 가 필요하다

규칙 문법

설정 파일 경로는 `/boot/system/settings/haikufw.conf` 이다.

지원 action:
- `allow`
- `deny`

지원 protocol:
- `tcp`
- `udp`
- `all`

지원 source 지정:
- `any`
- IPv4 host
- IPv4 CIDR
- IPv6 host
- IPv6 CIDR

지원 destination port:
- `any`
- 단일 포트: `22`
- 콤마 목록: `80,443`
- 범위: `60000-61000`
- 목록+범위 혼합: `53,80,443,60000-61000`

최소 예시:

default_inbound = deny

allow tcp from any to port 22
allow udp from any to port 53

전체 문법 조합 예시:

전체 protocol/source/port 조합 매트릭스는 `config/haikufw.conf.example`에
정리되어 있다.

대표적인 generic 예시:

default_inbound = deny

allow tcp from 203.0.113.0/24 to port 22
allow tcp from 203.0.113.10 to port 22
allow tcp from 2001:db8:100::/64 to port 80,443
allow udp from 203.0.113.0/24 to port 53,123
allow all from 198.51.100.0/24 to port 60000-61000
deny udp from 203.0.113.0/24 to port 53,123
deny all from 2001:db8:200::/64 to port 25,110,143

빌드

Haiku 머신에서 extract 한 release 루트 기준:

1. `./scripts/build-haikufw-haiku.sh`

이 빌드 스크립트는 다음을 수행한다.
- 패치된 Haiku network protocol 소스를 준비
- `haikufw_core` 빌드
- `haikufw` control driver 빌드
- patched `ipv4` 와 `ipv6` 빌드
- `haikufwctl` 빌드
- 결과물을 `out/haiku/`에 생성

설치

1. `./scripts/install-haikufw.sh install`
2. reboot

설치 시 다음 파일이 기록된다.
- `/boot/system/non-packaged/add-ons/kernel/generic/haikufw_core`
- `/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/haikufw`
- `/boot/system/non-packaged/add-ons/kernel/network/protocols/ipv4`
- `/boot/system/non-packaged/add-ons/kernel/network/protocols/ipv6`
- `/boot/system/non-packaged/bin/haikufwctl`
- `/boot/system/non-packaged/bin/haikufw-boot.sh`

설치 과정은 추가로 다음도 수행한다.
- `/boot/system/settings/haikufw.conf`가 없으면 기본 설정 생성
- `UserBootscript`에 boot block 등록
- installer state 디렉터리에 backup 저장

설치 후 검증

reboot 후 다음을 확인한다.

1. 디바이스 존재 확인:
   - `ls /dev/misc/haikufw`
2. 커널 상태 확인:
   - `haikufwctl status`
3. 관리 경로가 살아 있는지 확인:
   - 의도한 source network 에서 SSH 접속

라이브 설정 변경 절차

1. `/boot/system/settings/haikufw.conf` 수정
2. 문법 검사:
   - `haikufwctl check /boot/system/settings/haikufw.conf`
3. 재설치 없이 live reload:
   - `haikufwctl reload /boot/system/settings/haikufw.conf`
4. 런타임 상태 확인:
   - `haikufwctl status`

런타임 규칙만 초기화

- `haikufwctl clear`

이 명령은 커널 안의 활성 ruleset 만 비운다. 설치 파일을 제거하지 않고,
디스크의 설정 파일도 수정하지 않는다.

삭제

1. `./scripts/install-haikufw.sh uninstall`
2. reboot

삭제 시 다음이 제거된다.
- `/boot/system/non-packaged` 아래의 설치 파일
- `UserBootscript` 의 boot registration block

삭제 시 다음이 복구된다.
- backup 해 둔 stock `ipv4` / `ipv6` protocol module
- installer manifest 에 기록된 기존 파일

테스트

이 배포물에는 다음 검증 스크립트가 포함된다.

- host parser/compiler smoke:
  - `./scripts/test-host.sh`
- Haiku 내부 loopback TCP/UDP live smoke:
  - `./scripts/live-rule-smoke-haiku.sh`
- control host 에서의 reboot smoke:
  - `python3 ./scripts/reboot-smoke-remote.py --host <host> --password-file <file> --iterations 3 --settle-seconds 30`

운영 적용 순서

권장 rollout 절차:

1. 처음에는 `default_inbound = allow`로 시작
2. `check`, `reload`, `status` 동작 확인
3. 필요한 allow 규칙 추가:
   - SSH 관리 경로
   - 실제 서비스 포트
4. 그 다음 `default_inbound = deny`로 전환
5. 별도 클라이언트에서 실제로 필요한 포트만 열려 있는지 검증

현재 범위

현재 검증된 범위는 Haiku IPv4/IPv6 receive path 에서의 inbound TCP/UDP
필터링이다. 이 배포물은 그 구현과 검증 범위를 기준으로 제공된다.
