# KIMServer
<img src="docs/logo.svg" height=320>

KIMServer is an open-source, robust, scalable and extensible Instant Message platform written By C++.

The design of the project can refer to https://juejin.cn/post/7156134985504653349/#heading-45

## Compile and running

See [wiki/Compile](https://github.com/taroyuyu/KIMServer/wiki/Compile) to build the KIMServer.

See [wiki/Database-Schema](https://github.com/taroyuyu/KIMServer/wiki/Database-Schema) to configure database schema.

See [wiki/Configuration](https://github.com/taroyuyu/KIMServer/wiki/Configuration) to configure KIMServer instance.

## Getting support

* For bugs and feature requests [open an issue](https://github.com/taroyuyu/KIMServer/issues/new/choose).

## Helping out

* If you appreciate our work, please help spread the project! Sharing on Reddit, HN, and other communities helps more than you think.
* If you are a software developer, send us your pull requests with bug fixes and new features.
* If you are a client or frontend developer, help us develop the KIM Client on different platforms, such as iOS, Android, Mac, Windows, Linux, Web, et.
* If you are a UI/UX expert, help us polish the app UI.
* If you use the app and discover bugs or missing features, let us know by filing bug reports and feature requests. Vote for existing [feature requests](https://github.com/taroyuyu/KIMServer/issues?q=is%3Aissue+is%3Aopen+sort%3Areactions-%2B1-desc+label%3A%22feature+request%22) you find most valuable.

## Features

### Supported

- Multiple native platforms:
  - [iOS and Mac](https://github.com/taroyuyu/kim-iOS) (Objective-C)
  - [command line](source/Client)
- User features:
  - One-on-one and group messaging. 
  - Video and audio calling.
    - Support session initiation protocol and video calling technology. A set of session initiation protocol  needed for video calling is designed and implemented by reference SIP protocol.
  - All chats are synchronized across all devices.
  - Multi-device access.
  - Friend management.
  - Group management.

- Performance
  - Easy to extend. Cluster architecture is adopted to support multiple servers to form an INSTANT messaging system service cluster.
  - Dynamic allocated of service nodes. Service nodes can be allocated based on the load on the node and its distance from the client.

## Screenshots

### [iOS](https://github.com/taroyuyu/kim-iOS)

### [macOS](https://github.com/taroyuyu/kim-iOS)

#### SEO Strings

Words 'chat' and 'instant messaging' in Chinese, Russian, Persian and a few other languages.

- 聊天室 即时通讯
- чат мессенджер
- インスタントメッセージ
- 인스턴트 메신저
- Nhắn tin tức thời
* anlık mesajlaşma sohbet
* mensageiro instantâneo
* pesan instan
* mensajería instantánea
* চ্যাট ইন্সট্যান্ট মেসেজিং
* चैट त्वरित संदेश
* پیام رسان فوری
* تراسل فوري

## License

KIMServer is released under MIT License.
