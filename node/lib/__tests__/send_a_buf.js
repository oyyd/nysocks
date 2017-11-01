'use strict';

var _socket = require('../socket');

function main() {
  (0, _socket.startKcpuv)();

  var addr = '127.0.0.1';
  var senderPort = 8888;
  var receiverPort = 8889;
  var msg = 'hello';

  var sender = (0, _socket.create)();
  var receiver = (0, _socket.create)();

  (0, _socket.listen)(sender, senderPort, function (buf) {
    console.log('sender: ', buf);
  });

  (0, _socket.listen)(receiver, receiverPort, function (buf) {
    var content = buf.toString('utf8');
    console.log('receive: ', content);

    if (content === msg) {
      (0, _socket.close)(sender);
      (0, _socket.close)(receiver);
      (0, _socket.stopListen)(sender);
      (0, _socket.stopListen)(receiver);
      setTimeout(function () {
        (0, _socket.stopKcpuv)();
      }, 100);
    }
  });

  (0, _socket.setAddr)(sender, addr, receiverPort);
  (0, _socket.setAddr)(receiver, addr, senderPort);

  var buf = Buffer.from(msg);

  (0, _socket.send)(sender, buf, buf.length);
}

main();