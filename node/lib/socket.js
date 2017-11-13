'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.setAddr = exports.close = exports.send = exports.stopListen = exports.getPort = exports.bindListener = exports.listen = exports.destroy = undefined;
exports.create = create;
exports.createConnection = createConnection;
exports.startKcpuv = startKcpuv;
exports.stopKcpuv = stopKcpuv;

var _events = require('events');

var _events2 = _interopRequireDefault(_events);

var _addon = require('../../build/Release/addon.node');

var _addon2 = _interopRequireDefault(_addon);

var _utils = require('./utils');

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

var suite = (0, _utils.createBaseSuite)('_sess');
var wrap = suite.wrap;


_addon2.default.useDefaultLoop(true);

_addon2.default.initialize();

// const {
//   create, free,
//   listen, stopListen, initSend, send, bindClose, close,
//   initialize, destruct,
//   startLoop, destroyLoop,
// } = binding

function create() {
  var sess = _addon2.default.create();
  sess.event = new _events2.default();
  sess._sess = true;

  _addon2.default.bindClose(sess, function (errorMsg) {
    return sess.event.emit('close', errorMsg);
  });

  return sess;
}

var destroy = exports.destroy = wrap(function (sess) {
  _addon2.default.free(sess);
});

var listen = exports.listen = wrap(function (sess) {
  var port = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : 0;
  var onMessage = arguments[2];

  var errMsg = _addon2.default.listen(sess, port, onMessage);

  if (errMsg) {
    throw new Error(errMsg);
  }
});

var bindListener = exports.bindListener = wrap(function (sess, onMessage) {
  if (typeof onMessage !== 'function') {
    throw new Error('expe)ct "onMessage" to be a function');
  }

  _addon2.default.bindListen(sess, onMessage);
});

var getPort = exports.getPort = wrap(function (sess) {
  return _addon2.default.getPort(sess);
});

var stopListen = exports.stopListen = wrap(function (sess) {
  _addon2.default.stopListen(sess);
});

var send = exports.send = wrap(function (sess, buf) {
  _addon2.default.send(sess, buf, buf.length);
});

var close = exports.close = wrap(function (sess) {
  var sendCloseMsg = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : false;

  _addon2.default.close(sess, sendCloseMsg);
});

var setAddr = exports.setAddr = wrap(function (sess, address, port) {
  _addon2.default.initSend(sess, address, port);
});

function createConnection(targetAddress, targetPort, onMsg) {
  var sess = create();

  listen(sess, 0, onMsg);
  setAddr(sess, targetAddress, targetPort);

  return sess;
}

function startKcpuv() {
  _addon2.default.startLoop();
}

// TODO:
function stopKcpuv() {
  _addon2.default.destroyLoop();
  // binding.destruct()
}