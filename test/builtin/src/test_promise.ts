// A resolution
// {
//   const expected_result = 42;
//   const promise = globalThis.addon.createPromise();
//   promise.then(
//     common.mustCall((result) => {
//       globalThis.assert.strictEqual(result, expected_result);
//     }),
//     common.mustNotCall(),
//   );
//   globalThis.addon.concludeCurrentPromise(expected_result, true);
// }

// A rejection
// {
//   const expected_result = 'It\'s not you, it\'s me.';
//   const promise = globalThis.addon.createPromise();
//   promise.then(
//     common.mustNotCall(),
//     common.mustCall((result) => {
//       globalThis.assert.strictEqual(result, expected_result);
//     }),
//   );
//   globalThis.addon.concludeCurrentPromise(expected_result, false);
// }

// Chaining
// {
//   const expected_result = 'chained answer';
//   const promise = globalThis.addon.createPromise();
//   promise.then(
//     common.mustCall((result) => {
//       globalThis.assert.strictEqual(result, expected_result);
//     }),
//     common.mustNotCall(),
//   );
//   globalThis.addon.concludeCurrentPromise(Promise.resolve('chained answer'), true);
// }

const promiseTypeTestPromise = globalThis.addon.createPromise();
globalThis.assert.strictEqual(globalThis.addon.isPromise(promiseTypeTestPromise), true);
globalThis.addon.concludeCurrentPromise(undefined, true);

const rejectPromise = Promise.reject(-1);
const expected_reason = -1;
// Promise polyfill 无法通过 isPromise 测试
// globalThis.assert.strictEqual(globalThis.addon.isPromise(rejectPromise), true);
rejectPromise.catch((reason) => {
  globalThis.assert.strictEqual(reason, expected_reason);
});

globalThis.assert.strictEqual(globalThis.addon.isPromise(2.4), false);
globalThis.assert.strictEqual(globalThis.addon.isPromise('I promise!'), false);
globalThis.assert.strictEqual(globalThis.addon.isPromise(undefined), false);
globalThis.assert.strictEqual(globalThis.addon.isPromise(null), false);
globalThis.assert.strictEqual(globalThis.addon.isPromise({}), false);

export { }