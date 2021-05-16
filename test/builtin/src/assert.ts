class AssertionError extends Error {
  constructor(message?: string) {
    super(message);
    this.name = 'AssertionError';
  }
}

/**
 * Converts the input into a string. Objects, Sets and Maps are sorted so as to
 * make tests less flaky
 * @param v Value to be formatted
 */
function format(v: unknown) {
  return `${String(v)}`;
}

function assert(expr: unknown, msg = "") {
  if (!expr) {
    throw new AssertionError(msg);
  }
}
assert.ok = assert;

/**
 * Make an assertion that `actual` and `expected` are strictly equal.  If
 * not then throw.
 * ```ts
 * assertStrictEquals(1, 2)
 * ```
 */
assert.strictEqual = function strictEqual<T = unknown>(
  actual: T,
  expected: T,
  msg?: string,
) {
  if (Object.is(actual, expected)) {
    return;
  }

  let message = msg;

  if (!message) {
    message = `${format(actual)} !== ${format(expected)}`;
  }

  throw new AssertionError(message);
};

/**
 * Make an assertion that `actual` and `expected` are not strictly equal.
 * If the values are strictly equal then throw.
 * ```ts
 * assertNotStrictEquals(1, 1)
 * ```
 */
assert.notStrictEqual = function notStrictEqual<T = unknown>(
  actual: T,
  expected: T,
  msg?: string,
) {
  if (!Object.is(actual, expected)) {
    return;
  }

  throw new AssertionError(
    msg ?? `Expected "actual" to be strictly unequal to: ${format(actual)}\n`,
  );
};

assert.throws = function throws<T = void>(fn: () => T, msg?: string) {
  let doesThrow = false;
  let error = null;
  let message = msg;
  try {
    fn();
  } catch (e) {
    // if (e instanceof Error === false) {
    //   throw new AssertionError('A non-Error object was thrown.');
    // }
    doesThrow = true;
    error = e;
  }
  if (!doesThrow) {
    message = `Expected function to throw${msg ? `: ${msg}` : '.'}`;
    throw new AssertionError(message);
  }
  return error;
};

globalThis.assert = assert;

export { }