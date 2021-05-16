globalThis.addon.RunCallback((msg: unknown) => {
  globalThis.assert.strictEqual(msg, 'hello world');
});

function testRecv(desiredRecv: unknown) {
  globalThis.addon.RunCallbackWithRecv(function () {
    globalThis.assert.strictEqual(this, desiredRecv);
  }, desiredRecv);
}

// V8 支持原样传递 this，但是 JavaScriptCore 只支持对象，就算强制转换为对象后也无法通过 === 判断
// testRecv(undefined);
// testRecv(null);
// testRecv(5);
// testRecv(true);
// testRecv('Hello');
testRecv([]);
testRecv({});

export { }