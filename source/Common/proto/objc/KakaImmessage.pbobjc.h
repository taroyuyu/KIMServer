// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: KakaIMMessage.proto

// This CPP symbol can be defined to use imports that match up to the framework
// imports needed when using CocoaPods.
#if !defined(GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS)
 #define GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS 0
#endif

#if GPB_USE_PROTOBUF_FRAMEWORK_IMPORTS
 #import <Protobuf/GPBProtocolBuffers.h>
#else
 #import "GPBProtocolBuffers.h"
#endif

#if GOOGLE_PROTOBUF_OBJC_VERSION < 30002
#error This file was generated by a newer version of protoc which is incompatible with your Protocol Buffer library sources.
#endif
#if 30002 < GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION
#error This file was generated by an older version of protoc which is incompatible with your Protocol Buffer library sources.
#endif

// @@protoc_insertion_point(imports)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

CF_EXTERN_C_BEGIN

@class FriendList;
@class FriendListItem;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Enum UserGenderType

typedef GPB_ENUM(UserGenderType) {
  /** 男 */
  UserGenderType_Male = 1,

  /** 女 */
  UserGenderType_Female = 2,

  /** 未知 */
  UserGenderType_Unkown = 3,
};

GPBEnumDescriptor *UserGenderType_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL UserGenderType_IsValidValue(int32_t value);

#pragma mark - Enum ResponseLoginMessage_LoginState

typedef GPB_ENUM(ResponseLoginMessage_LoginState) {
  /** 登录成功 */
  ResponseLoginMessage_LoginState_Success = 1,

  /** 登录失败 */
  ResponseLoginMessage_LoginState_Failure = 2,
};

GPBEnumDescriptor *ResponseLoginMessage_LoginState_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL ResponseLoginMessage_LoginState_IsValidValue(int32_t value);

#pragma mark - Enum ResponseLoginMessage_FailureError

typedef GPB_ENUM(ResponseLoginMessage_FailureError) {
  /** 帐号或密码错误 */
  ResponseLoginMessage_FailureError_WrongAccountOrPassword = 1,

  /** 服务器内部错误 */
  ResponseLoginMessage_FailureError_InternalError = 2,
};

GPBEnumDescriptor *ResponseLoginMessage_FailureError_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL ResponseLoginMessage_FailureError_IsValidValue(int32_t value);

#pragma mark - Enum RegisterMessage_UserSex

typedef GPB_ENUM(RegisterMessage_UserSex) {
  /** 男生 */
  RegisterMessage_UserSex_Male = 1,

  /** 女生 */
  RegisterMessage_UserSex_Female = 2,

  /** 未知 */
  RegisterMessage_UserSex_Unkown = 3,
};

GPBEnumDescriptor *RegisterMessage_UserSex_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL RegisterMessage_UserSex_IsValidValue(int32_t value);

#pragma mark - Enum ResponseRegisterMessage_RegisterState

typedef GPB_ENUM(ResponseRegisterMessage_RegisterState) {
  /** 登录成功 */
  ResponseRegisterMessage_RegisterState_Success = 1,

  /** 登录失败 */
  ResponseRegisterMessage_RegisterState_Failure = 2,
};

GPBEnumDescriptor *ResponseRegisterMessage_RegisterState_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL ResponseRegisterMessage_RegisterState_IsValidValue(int32_t value);

#pragma mark - Enum ResponseRegisterMessage_FailureError

typedef GPB_ENUM(ResponseRegisterMessage_FailureError) {
  /** 帐号已经存在 */
  ResponseRegisterMessage_FailureError_AccountAlreadyExists = 1,

  /** 服务器内部错误 */
  ResponseRegisterMessage_FailureError_InternalError = 2,
};

GPBEnumDescriptor *ResponseRegisterMessage_FailureError_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL ResponseRegisterMessage_FailureError_IsValidValue(int32_t value);

#pragma mark - Enum LogoutMessage_OfflineMailState

typedef GPB_ENUM(LogoutMessage_OfflineMailState) {
  /** 开启 */
  LogoutMessage_OfflineMailState_Open = 1,

  /** 关闭 */
  LogoutMessage_OfflineMailState_Close = 2,
};

GPBEnumDescriptor *LogoutMessage_OfflineMailState_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL LogoutMessage_OfflineMailState_IsValidValue(int32_t value);

#pragma mark - Enum ResponseLogoutMessage_OfflineMailState

typedef GPB_ENUM(ResponseLogoutMessage_OfflineMailState) {
  /** 开启 */
  ResponseLogoutMessage_OfflineMailState_Open = 1,

  /** 关闭 */
  ResponseLogoutMessage_OfflineMailState_Close = 2,
};

GPBEnumDescriptor *ResponseLogoutMessage_OfflineMailState_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL ResponseLogoutMessage_OfflineMailState_IsValidValue(int32_t value);

#pragma mark - Enum BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer

typedef GPB_ENUM(BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer) {
  /** 接受 */
  BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_BuildingRelationshipAnswerAccept = 1,

  /** 拒绝 */
  BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_BuildingRelationshipAnswerReject = 2,
};

GPBEnumDescriptor *BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_IsValidValue(int32_t value);

#pragma mark - Enum DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse

typedef GPB_ENUM(DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse) {
  /** 拆除成功 */
  DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_DestoryingRelationshipResponseSuccess = 1,

  /** 拆除失败 */
  DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_DestoryingRelationshipResponseFailed = 2,
};

GPBEnumDescriptor *DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_IsValidValue(int32_t value);

#pragma mark - Enum OnlineStateMessage_OnlineState

typedef GPB_ENUM(OnlineStateMessage_OnlineState) {
  /** 在线 */
  OnlineStateMessage_OnlineState_Online = 1,

  /** 离线 */
  OnlineStateMessage_OnlineState_Offline = 2,

  /** 隐身 */
  OnlineStateMessage_OnlineState_Invisible = 3,
};

GPBEnumDescriptor *OnlineStateMessage_OnlineState_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL OnlineStateMessage_OnlineState_IsValidValue(int32_t value);

#pragma mark - Enum NotificationMessage_NotificationMessageType

typedef GPB_ENUM(NotificationMessage_NotificationMessageType) {
  /** 系统通知 */
  NotificationMessage_NotificationMessageType_SystemNotificationMessageType = 1,
};

GPBEnumDescriptor *NotificationMessage_NotificationMessageType_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL NotificationMessage_NotificationMessageType_IsValidValue(int32_t value);

#pragma mark - Enum UpdateUserVCardMessageResponse_UpdateUserVCardStateType

typedef GPB_ENUM(UpdateUserVCardMessageResponse_UpdateUserVCardStateType) {
  /** 更新成功 */
  UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Success = 1,

  /** 更新失败 */
  UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Failure = 2,
};

GPBEnumDescriptor *UpdateUserVCardMessageResponse_UpdateUserVCardStateType_EnumDescriptor(void);

/**
 * Checks to see if the given value is defined by the enum or was not known at
 * the time this source was generated.
 **/
BOOL UpdateUserVCardMessageResponse_UpdateUserVCardStateType_IsValidValue(int32_t value);

#pragma mark - KakaImmessageRoot

/**
 * Exposes the extension registry for this file.
 *
 * The base class provides:
 * @code
 *   + (GPBExtensionRegistry *)extensionRegistry;
 * @endcode
 * which is a @c GPBExtensionRegistry that includes all the extensions defined by
 * this file and all files that it depends on.
 **/
@interface KakaImmessageRoot : GPBRootObject
@end

#pragma mark - RequestSessionIDMessage

/**
 * 请求会话ID消息
 **/
@interface RequestSessionIDMessage : GPBMessage

@end

#pragma mark - ResponseSessionIDMessage

typedef GPB_ENUM(ResponseSessionIDMessage_FieldNumber) {
  ResponseSessionIDMessage_FieldNumber_SessionId = 1,
};

/**
 * 响应会话ID消息
 **/
@interface ResponseSessionIDMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

@end

#pragma mark - LoginMessage

typedef GPB_ENUM(LoginMessage_FieldNumber) {
  LoginMessage_FieldNumber_SessionId = 1,
  LoginMessage_FieldNumber_UserAccount = 2,
  LoginMessage_FieldNumber_UserPassword = 3,
};

/**
 * 登录消息
 **/
@interface LoginMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 账户 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

/** 密码 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userPassword;
/** Test to see if @c userPassword has been set. */
@property(nonatomic, readwrite) BOOL hasUserPassword;

@end

#pragma mark - ResponseLoginMessage

typedef GPB_ENUM(ResponseLoginMessage_FieldNumber) {
  ResponseLoginMessage_FieldNumber_SessionId = 1,
  ResponseLoginMessage_FieldNumber_LoginState = 2,
  ResponseLoginMessage_FieldNumber_FailureError = 3,
};

/**
 * 响应登录消息
 **/
@interface ResponseLoginMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 登录状态 */
@property(nonatomic, readwrite) ResponseLoginMessage_LoginState loginState;

@property(nonatomic, readwrite) BOOL hasLoginState;
/** 错误原因 */
@property(nonatomic, readwrite) ResponseLoginMessage_FailureError failureError;

@property(nonatomic, readwrite) BOOL hasFailureError;
@end

#pragma mark - RegisterMessage

typedef GPB_ENUM(RegisterMessage_FieldNumber) {
  RegisterMessage_FieldNumber_SessionId = 1,
  RegisterMessage_FieldNumber_UserAccount = 2,
  RegisterMessage_FieldNumber_UserPassword = 3,
  RegisterMessage_FieldNumber_UserNickName = 4,
  RegisterMessage_FieldNumber_Sex = 5,
};

/**
 * 注册消息
 **/
@interface RegisterMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 账户 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

/** 密码 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userPassword;
/** Test to see if @c userPassword has been set. */
@property(nonatomic, readwrite) BOOL hasUserPassword;

/** 昵称 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userNickName;
/** Test to see if @c userNickName has been set. */
@property(nonatomic, readwrite) BOOL hasUserNickName;

/** 用户性别 */
@property(nonatomic, readwrite) RegisterMessage_UserSex sex;

@property(nonatomic, readwrite) BOOL hasSex;
@end

#pragma mark - ResponseRegisterMessage

typedef GPB_ENUM(ResponseRegisterMessage_FieldNumber) {
  ResponseRegisterMessage_FieldNumber_SessionId = 1,
  ResponseRegisterMessage_FieldNumber_RegisterState = 2,
  ResponseRegisterMessage_FieldNumber_FailureError = 3,
};

/**
 * 响应注册消息
 **/
@interface ResponseRegisterMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 注册状态 */
@property(nonatomic, readwrite) ResponseRegisterMessage_RegisterState registerState;

@property(nonatomic, readwrite) BOOL hasRegisterState;
/** 错误原因 */
@property(nonatomic, readwrite) ResponseRegisterMessage_FailureError failureError;

@property(nonatomic, readwrite) BOOL hasFailureError;
@end

#pragma mark - HeartBeatMessage

typedef GPB_ENUM(HeartBeatMessage_FieldNumber) {
  HeartBeatMessage_FieldNumber_SessionId = 1,
  HeartBeatMessage_FieldNumber_Timestamp = 2,
};

/**
 * 心跳包
 **/
@interface HeartBeatMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 时间戳 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *timestamp;
/** Test to see if @c timestamp has been set. */
@property(nonatomic, readwrite) BOOL hasTimestamp;

@end

#pragma mark - ResponseHeartBeatMessage

typedef GPB_ENUM(ResponseHeartBeatMessage_FieldNumber) {
  ResponseHeartBeatMessage_FieldNumber_SessionId = 1,
  ResponseHeartBeatMessage_FieldNumber_Timestamp = 2,
};

/**
 * 响应心跳包
 **/
@interface ResponseHeartBeatMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 时间戳 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *timestamp;
/** Test to see if @c timestamp has been set. */
@property(nonatomic, readwrite) BOOL hasTimestamp;

@end

#pragma mark - LogoutMessage

typedef GPB_ENUM(LogoutMessage_FieldNumber) {
  LogoutMessage_FieldNumber_SessionId = 1,
  LogoutMessage_FieldNumber_OfflineMaileState = 2,
};

/**
 * 离线消息
 **/
@interface LogoutMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 离线邮箱状态 */
@property(nonatomic, readwrite) LogoutMessage_OfflineMailState offlineMaileState;

@property(nonatomic, readwrite) BOOL hasOfflineMaileState;
@end

#pragma mark - ResponseLogoutMessage

typedef GPB_ENUM(ResponseLogoutMessage_FieldNumber) {
  ResponseLogoutMessage_FieldNumber_SessionId = 1,
  ResponseLogoutMessage_FieldNumber_OfflineMaileState = 2,
};

/**
 * 响应离线消息
 **/
@interface ResponseLogoutMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 离线邮箱状态 */
@property(nonatomic, readwrite) ResponseLogoutMessage_OfflineMailState offlineMaileState;

@property(nonatomic, readwrite) BOOL hasOfflineMaileState;
@end

#pragma mark - BuildingRelationshipRequestMessage

typedef GPB_ENUM(BuildingRelationshipRequestMessage_FieldNumber) {
  BuildingRelationshipRequestMessage_FieldNumber_SessionId = 1,
  BuildingRelationshipRequestMessage_FieldNumber_SponsorAccount = 2,
  BuildingRelationshipRequestMessage_FieldNumber_UserAccount = 3,
  BuildingRelationshipRequestMessage_FieldNumber_MessageId = 4,
};

@interface BuildingRelationshipRequestMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 发起者ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sponsorAccount;
/** Test to see if @c sponsorAccount has been set. */
@property(nonatomic, readwrite) BOOL hasSponsorAccount;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

/** 消息ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *messageId;
/** Test to see if @c messageId has been set. */
@property(nonatomic, readwrite) BOOL hasMessageId;

@end

#pragma mark - BuildingRelationshipAnswerMessage

typedef GPB_ENUM(BuildingRelationshipAnswerMessage_FieldNumber) {
  BuildingRelationshipAnswerMessage_FieldNumber_SessionId = 1,
  BuildingRelationshipAnswerMessage_FieldNumber_UserAccount = 2,
  BuildingRelationshipAnswerMessage_FieldNumber_SponsorAccount = 3,
  BuildingRelationshipAnswerMessage_FieldNumber_Answer = 4,
  BuildingRelationshipAnswerMessage_FieldNumber_MessageId = 5,
};

@interface BuildingRelationshipAnswerMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

/** 发起者ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sponsorAccount;
/** Test to see if @c sponsorAccount has been set. */
@property(nonatomic, readwrite) BOOL hasSponsorAccount;

/** 回复 */
@property(nonatomic, readwrite) BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer answer;

@property(nonatomic, readwrite) BOOL hasAnswer;
/** 消息ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *messageId;
/** Test to see if @c messageId has been set. */
@property(nonatomic, readwrite) BOOL hasMessageId;

@end

#pragma mark - DestroyingRelationshipRequestMessage

typedef GPB_ENUM(DestroyingRelationshipRequestMessage_FieldNumber) {
  DestroyingRelationshipRequestMessage_FieldNumber_SessionId = 1,
  DestroyingRelationshipRequestMessage_FieldNumber_SponsorAccount = 2,
  DestroyingRelationshipRequestMessage_FieldNumber_UserAccount = 3,
};

@interface DestroyingRelationshipRequestMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 发起者ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sponsorAccount;
/** Test to see if @c sponsorAccount has been set. */
@property(nonatomic, readwrite) BOOL hasSponsorAccount;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

@end

#pragma mark - DestoryingRelationshipResponseMessage

typedef GPB_ENUM(DestoryingRelationshipResponseMessage_FieldNumber) {
  DestoryingRelationshipResponseMessage_FieldNumber_SessionId = 1,
  DestoryingRelationshipResponseMessage_FieldNumber_SponsorAccount = 2,
  DestoryingRelationshipResponseMessage_FieldNumber_UserAccount = 3,
  DestoryingRelationshipResponseMessage_FieldNumber_Response = 4,
};

/**
 * 拆除好友关系回复消息
 **/
@interface DestoryingRelationshipResponseMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 发起者ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sponsorAccount;
/** Test to see if @c sponsorAccount has been set. */
@property(nonatomic, readwrite) BOOL hasSponsorAccount;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

/** 响应 */
@property(nonatomic, readwrite) DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse response;

@property(nonatomic, readwrite) BOOL hasResponse;
@end

#pragma mark - FriendListRequestMessage

typedef GPB_ENUM(FriendListRequestMessage_FieldNumber) {
  FriendListRequestMessage_FieldNumber_SessionId = 1,
};

/**
 * 请求好友列表消息
 **/
@interface FriendListRequestMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

@end

#pragma mark - FriendListItem

typedef GPB_ENUM(FriendListItem_FieldNumber) {
  FriendListItem_FieldNumber_FriendAccount = 1,
};

@interface FriendListItem : GPBMessage

/** 好友ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *friendAccount;
/** Test to see if @c friendAccount has been set. */
@property(nonatomic, readwrite) BOOL hasFriendAccount;

@end

#pragma mark - FriendList

typedef GPB_ENUM(FriendList_FieldNumber) {
  FriendList_FieldNumber_ItemCount = 1,
  FriendList_FieldNumber_ItemArray = 2,
};

@interface FriendList : GPBMessage

/** 记录条数 */
@property(nonatomic, readwrite) int32_t itemCount;

@property(nonatomic, readwrite) BOOL hasItemCount;
/** 好友项 */
@property(nonatomic, readwrite, strong, null_resettable) NSMutableArray<FriendListItem*> *itemArray;
/** The number of items in @c itemArray without causing the array to be created. */
@property(nonatomic, readonly) NSUInteger itemArray_Count;

@end

#pragma mark - FriendListResponseMessage

typedef GPB_ENUM(FriendListResponseMessage_FieldNumber) {
  FriendListResponseMessage_FieldNumber_SessionId = 1,
  FriendListResponseMessage_FieldNumber_FriendList = 2,
};

/**
 * 响应好友列表请求消息
 **/
@interface FriendListResponseMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 好友列表 */
@property(nonatomic, readwrite, strong, null_resettable) FriendList *friendList;
/** Test to see if @c friendList has been set. */
@property(nonatomic, readwrite) BOOL hasFriendList;

@end

#pragma mark - OnlineStateMessage

typedef GPB_ENUM(OnlineStateMessage_FieldNumber) {
  OnlineStateMessage_FieldNumber_SessionId = 1,
  OnlineStateMessage_FieldNumber_UserAccount = 2,
  OnlineStateMessage_FieldNumber_UserState = 3,
};

/**
 * 在线消息
 **/
@interface OnlineStateMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userAccount;
/** Test to see if @c userAccount has been set. */
@property(nonatomic, readwrite) BOOL hasUserAccount;

/** 用户状态 */
@property(nonatomic, readwrite) OnlineStateMessage_OnlineState userState;

@property(nonatomic, readwrite) BOOL hasUserState;
@end

#pragma mark - ChatMessage

typedef GPB_ENUM(ChatMessage_FieldNumber) {
  ChatMessage_FieldNumber_SessionId = 1,
  ChatMessage_FieldNumber_SenderId = 2,
  ChatMessage_FieldNumber_ReceiverId = 3,
  ChatMessage_FieldNumber_Content = 4,
  ChatMessage_FieldNumber_Timestamp = 5,
  ChatMessage_FieldNumber_MessageId = 6,
};

/**
 * 聊天消息
 **/
@interface ChatMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 发送者 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *senderId;
/** Test to see if @c senderId has been set. */
@property(nonatomic, readwrite) BOOL hasSenderId;

/** 接受者ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *receiverId;
/** Test to see if @c receiverId has been set. */
@property(nonatomic, readwrite) BOOL hasReceiverId;

/** 消息内容 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *content;
/** Test to see if @c content has been set. */
@property(nonatomic, readwrite) BOOL hasContent;

/** 时间戳 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *timestamp;
/** Test to see if @c timestamp has been set. */
@property(nonatomic, readwrite) BOOL hasTimestamp;

/** 消息ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *messageId;
/** Test to see if @c messageId has been set. */
@property(nonatomic, readwrite) BOOL hasMessageId;

@end

#pragma mark - NotificationMessage

typedef GPB_ENUM(NotificationMessage_FieldNumber) {
  NotificationMessage_FieldNumber_Type = 1,
  NotificationMessage_FieldNumber_MessageId = 2,
};

@interface NotificationMessage : GPBMessage

/** 通知类型 */
@property(nonatomic, readwrite) NotificationMessage_NotificationMessageType type;

@property(nonatomic, readwrite) BOOL hasType;
/** 消息ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *messageId;
/** Test to see if @c messageId has been set. */
@property(nonatomic, readwrite) BOOL hasMessageId;

@end

#pragma mark - PullOfflineMessage

typedef GPB_ENUM(PullOfflineMessage_FieldNumber) {
  PullOfflineMessage_FieldNumber_SessionId = 1,
  PullOfflineMessage_FieldNumber_MessageId = 2,
};

@interface PullOfflineMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 拉取在messageID之后的消息 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *messageId;
/** Test to see if @c messageId has been set. */
@property(nonatomic, readwrite) BOOL hasMessageId;

@end

#pragma mark - FetchUserVCardMessage

typedef GPB_ENUM(FetchUserVCardMessage_FieldNumber) {
  FetchUserVCardMessage_FieldNumber_SessionId = 1,
  FetchUserVCardMessage_FieldNumber_UserId = 2,
};

@interface FetchUserVCardMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userId;
/** Test to see if @c userId has been set. */
@property(nonatomic, readwrite) BOOL hasUserId;

@end

#pragma mark - UserVCardResponseMessage

typedef GPB_ENUM(UserVCardResponseMessage_FieldNumber) {
  UserVCardResponseMessage_FieldNumber_SessionId = 1,
  UserVCardResponseMessage_FieldNumber_UserId = 2,
  UserVCardResponseMessage_FieldNumber_Nickname = 3,
  UserVCardResponseMessage_FieldNumber_Gender = 4,
  UserVCardResponseMessage_FieldNumber_Mood = 5,
  UserVCardResponseMessage_FieldNumber_Avator = 6,
};

@interface UserVCardResponseMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 用户ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *userId;
/** Test to see if @c userId has been set. */
@property(nonatomic, readwrite) BOOL hasUserId;

/** 昵称 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *nickname;
/** Test to see if @c nickname has been set. */
@property(nonatomic, readwrite) BOOL hasNickname;

/** 性别 */
@property(nonatomic, readwrite) UserGenderType gender;

@property(nonatomic, readwrite) BOOL hasGender;
/** 心情 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *mood;
/** Test to see if @c mood has been set. */
@property(nonatomic, readwrite) BOOL hasMood;

/** 头像 */
@property(nonatomic, readwrite, copy, null_resettable) NSData *avator;
/** Test to see if @c avator has been set. */
@property(nonatomic, readwrite) BOOL hasAvator;

@end

#pragma mark - UpdateUserVCardMessage

typedef GPB_ENUM(UpdateUserVCardMessage_FieldNumber) {
  UpdateUserVCardMessage_FieldNumber_SessionId = 1,
  UpdateUserVCardMessage_FieldNumber_Nickname = 2,
  UpdateUserVCardMessage_FieldNumber_Gender = 3,
  UpdateUserVCardMessage_FieldNumber_Mood = 4,
  UpdateUserVCardMessage_FieldNumber_Avator = 5,
};

@interface UpdateUserVCardMessage : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 昵称 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *nickname;
/** Test to see if @c nickname has been set. */
@property(nonatomic, readwrite) BOOL hasNickname;

/** 性别 */
@property(nonatomic, readwrite) UserGenderType gender;

@property(nonatomic, readwrite) BOOL hasGender;
/** 心情 */
@property(nonatomic, readwrite, copy, null_resettable) NSString *mood;
/** Test to see if @c mood has been set. */
@property(nonatomic, readwrite) BOOL hasMood;

/** 头像 */
@property(nonatomic, readwrite, copy, null_resettable) NSData *avator;
/** Test to see if @c avator has been set. */
@property(nonatomic, readwrite) BOOL hasAvator;

@end

#pragma mark - UpdateUserVCardMessageResponse

typedef GPB_ENUM(UpdateUserVCardMessageResponse_FieldNumber) {
  UpdateUserVCardMessageResponse_FieldNumber_SessionId = 1,
  UpdateUserVCardMessageResponse_FieldNumber_State = 2,
};

@interface UpdateUserVCardMessageResponse : GPBMessage

/** 会话ID */
@property(nonatomic, readwrite, copy, null_resettable) NSString *sessionId;
/** Test to see if @c sessionId has been set. */
@property(nonatomic, readwrite) BOOL hasSessionId;

/** 更新操作执行结果 */
@property(nonatomic, readwrite) UpdateUserVCardMessageResponse_UpdateUserVCardStateType state;

@property(nonatomic, readwrite) BOOL hasState;
@end

NS_ASSUME_NONNULL_END

CF_EXTERN_C_END

#pragma clang diagnostic pop

// @@protoc_insertion_point(global_scope)
