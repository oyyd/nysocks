'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.setAddr = exports.close = exports.send = exports.stopListen = exports.getPort = exports.bindListener = exports.listen = exports.destroy = undefined;

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

exports.create = create;
exports.createConnection = createConnection;
exports.startKcpuv = startKcpuv;
exports.stopKcpuv = stopKcpuv;

var _events = require('events');

var _events2 = _interopRequireDefault(_events);

var _addon = require('../../build/Release/addon.node');

var _addon2 = _interopRequireDefault(_addon);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

_addon2.default.useDefaultLoop(true);

_addon2.default.initialize();

// const {
//   create, free,
//   listen, stopListen, initSend, send, bindClose, close,
//   initialize, destruct,
//   startLoop, destroyLoop,
// } = binding

function checkValidSocket(obj) {
  if ((typeof obj === 'undefined' ? 'undefined' : _typeof(obj)) !== 'object' || !obj.isKcpuvSocket) {
    throw new Error('invalid kcpuv socket');
  }
}

function wrap(func) {
  return function (obj) {
    for (var _len = arguments.length, args = Array(_len > 1 ? _len - 1 : 0), _key = 1; _key < _len; _key++) {
      args[_key - 1] = arguments[_key];
    }

    checkValidSocket(obj);
    return func.apply(undefined, [obj].concat(args));
  };
}

function create() {
  var sess = _addon2.default.create();
  sess.event = new _events2.default();
  sess.isKcpuvSocket = true;

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