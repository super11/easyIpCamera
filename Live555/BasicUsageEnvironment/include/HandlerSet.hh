/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2012 Live Networks, Inc.  All rights reserved.
// Basic Usage Environment: for a simple, non-scripted, console application
// C++ header

#ifndef _HANDLER_SET_HH
#define _HANDLER_SET_HH

#ifndef _BOOLEAN_HH
#include "Boolean.hh"
#endif

////////// HandlerSet (etc.) definition //////////
/*
套接口描述类，在构造和析构时处理好链表的维护
*/
class HandlerDescriptor {
  HandlerDescriptor(HandlerDescriptor* nextHandler);
  virtual ~HandlerDescriptor();

public:
  /// 套接字句柄
  int socketNum;
  /// 关心事件
  int conditionSet;
  /// 回调函数
  TaskScheduler::BackgroundHandlerProc* handlerProc;
  /// 回调参数
  void* clientData;

private:
  // Descriptors are linked together in a doubly-linked list:
  friend class HandlerSet;
  friend class HandlerIterator;
  HandlerDescriptor* fNextHandler;
  HandlerDescriptor* fPrevHandler;
};

/// 容器类
class HandlerSet {
public:
  HandlerSet();
  virtual ~HandlerSet();
  /// 根据socketNum,增加或重设HandlerDescriptor
  void assignHandler(int socketNum, int conditionSet, TaskScheduler::BackgroundHandlerProc* handlerProc, void* clientData);
  /// 删除socketNum对应的HandlerDescriptor
  void clearHandler(int socketNum);
  /// 重设，对套接口句柄进行修改，其他参数不改变
  void moveHandler(int oldSocketNum, int newSocketNum);

private:
  HandlerDescriptor* lookupHandler(int socketNum);

private:
  friend class HandlerIterator;
  HandlerDescriptor fHandlers;
};
/// 枚举器
class HandlerIterator {
public:
  HandlerIterator(HandlerSet& handlerSet);
  virtual ~HandlerIterator();

  HandlerDescriptor* next(); // returns NULL if none
  void reset();

private:
  HandlerSet& fOurSet;
  HandlerDescriptor* fNextPtr;
};

#endif
