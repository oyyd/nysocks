'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.create = create;
exports.destroy = destroy;
exports.listen = listen;
exports.stopListen = stopListen;
exports.send = send;
exports.close = close;
exports.setAddr = setAddr;
exports.startKcpuv = startKcpuv;
exports.stopKcpuv = stopKcpuv;

var _events = require('events');

var _events2 = _interopRequireDefault(_events);

var _addon = require('../../build/Release/addon.node');

var _addon2 = _interopRequireDefault(_addon);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

_addon2.default.useDefaultLoop(true);

// const {
//   create, free,
//   listen, stopListen, initSend, send, bindClose, close,
//   initialize, destruct,
//   startLoop, destroyLoop,
// } = binding

function create() {
  var sess = _addon2.default.create();
  sess.event = new _events2.default();

  _addon2.default.bindClose(sess, function () {
    return sess.event.emit('close');
  });

  return sess;
}

function destroy(sess) {
  _addon2.default.free(sess);
}

function listen(sess) {
  var port = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : 0;
  var onMessage = arguments[2];

  _addon2.default.listen(sess, port, onMessage);
}

function stopListen(sess) {
  _addon2.default.stopListen(sess);
}

function send(sess, buf) {
  _addon2.default.send(sess, buf, buf.length);
}

function close(sess) {
  _addon2.default.close(sess);
}

function setAddr(sess, address, port) {
  _addon2.default.initSend(sess, address, port);
}

function startKcpuv() {
  _addon2.default.initialize();
  _addon2.default.startLoop();
}

function stopKcpuv() {
  _addon2.default.destroyLoop();
  _addon2.default.destruct();
}