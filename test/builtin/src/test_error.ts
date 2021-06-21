const theError = new Error('Some error');
const theTypeError = new TypeError('Some type error');
const theSyntaxError = new SyntaxError('Some syntax error');
const theRangeError = new RangeError('Some type error');
const theReferenceError = new ReferenceError('Some reference error');
const theURIError = new URIError('Some URI error');
const theEvalError = new EvalError('Some eval error');

class MyError extends Error { }
const myError = new MyError('Some MyError');

// Test that native error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theError), true);

// Test that native type error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theTypeError), true);

// Test that native syntax error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theSyntaxError), true);

// Test that native range error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theRangeError), true);

// Test that native reference error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theReferenceError), true);

// Test that native URI error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theURIError), true);

// Test that native eval error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(theEvalError), true);

// Test that class derived from native error is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError(myError), true);

// Test that non-error object is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError({}), false);

// Test that non-error primitive is correctly classed
globalThis.assert.strictEqual(globalThis.addon.checkError('non-object'), false);

globalThis.assert.throws(() => {
  globalThis.addon.throwExistingError();
});

globalThis.assert.throws(() => {
  globalThis.addon.throwError();
});

globalThis.assert.throws(() => {
  globalThis.addon.throwRangeError();
});

globalThis.assert.throws(() => {
  globalThis.addon.throwTypeError();
});

[42, {}, [], Symbol('xyzzy'), true, 'ball', undefined, null, NaN]
  .forEach((value) => globalThis.assert.throws(
    () => globalThis.addon.throwArbitrary(value),
    // (err) => {
    //   globalThis.assert.strictEqual(err, value);
    //   return true;
    // }
  ));

globalThis.assert.throws(
  () => globalThis.addon.throwErrorCode(),
  //   {
  //     code: 'ERR_TEST_CODE',
  //     message: 'Error [error]'
  //   }
);

globalThis.assert.throws(
  () => globalThis.addon.throwRangeErrorCode(),
  //   {
  //     code: 'ERR_TEST_CODE',
  //     message: 'RangeError [range error]',
  //   },
);

globalThis.assert.throws(
  () => globalThis.addon.throwTypeErrorCode(),
  //   {
  //     code: 'ERR_TEST_CODE',
  //     message: 'TypeError [type error]',
  //   },
);

let error = globalThis.addon.createError();
globalThis.assert.ok(error instanceof Error, 'expected error to be an instance of Error');
globalThis.assert.strictEqual(error.message, 'error');

error = globalThis.addon.createRangeError();
globalThis.assert.ok(error instanceof RangeError,
  'expected error to be an instance of RangeError');
globalThis.assert.strictEqual(error.message, 'range error');

error = globalThis.addon.createTypeError();
globalThis.assert.ok(error instanceof TypeError,
  'expected error to be an instance of TypeError');
globalThis.assert.strictEqual(error.message, 'type error');

error = globalThis.addon.createErrorCode();
globalThis.assert.ok(error instanceof Error, 'expected error to be an instance of Error');
globalThis.assert.strictEqual(error.code, 'ERR_TEST_CODE');
globalThis.assert.strictEqual(error.message, 'Error [error]');
globalThis.assert.strictEqual(error.name, 'Error');

error = globalThis.addon.createRangeErrorCode();
globalThis.assert.ok(error instanceof RangeError,
  'expected error to be an instance of RangeError');
globalThis.assert.strictEqual(error.message, 'RangeError [range error]');
globalThis.assert.strictEqual(error.code, 'ERR_TEST_CODE');
globalThis.assert.strictEqual(error.name, 'RangeError');

error = globalThis.addon.createTypeErrorCode();
globalThis.assert.ok(error instanceof TypeError,
  'expected error to be an instance of TypeError');
globalThis.assert.strictEqual(error.message, 'TypeError [type error]');
globalThis.assert.strictEqual(error.code, 'ERR_TEST_CODE');
globalThis.assert.strictEqual(error.name, 'TypeError');

export { }