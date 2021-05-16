function func1() {
  return 1;
}
globalThis.assert.strictEqual(globalThis.addon.TestCall(func1), 1);

function func2() {
  //   console.log('hello world!');
  return null;
}
globalThis.assert.strictEqual(globalThis.addon.TestCall(func2), null);

function func3(input: number) {
  return input + 1;
}
globalThis.assert.strictEqual(globalThis.addon.TestCall(func3, 1), 2);

function func4(input: number) {
  return func3(input);
}
globalThis.assert.strictEqual(globalThis.addon.TestCall(func4, 1), 2);

globalThis.assert.strictEqual(globalThis.addon.TestName.name, 'Name');
// 不支持截断
globalThis.assert.strictEqual(globalThis.addon.TestNameShort.name, 'Name_extra');

// let tracked_function = globalThis.addon.MakeTrackedFunction(common.mustCall());
// globalThis.assert(!!tracked_function);
// tracked_function = null;
// global.gc();

// globalThis.assert.deepStrictEqual(globalThis.addon.TestCreateFunctionParameters(), {
//   envIsNull: 'Invalid argument',
//   nameIsNull: 'napi_ok',
//   cbIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument'
// });

export { }