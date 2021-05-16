const testSym = Symbol('test');

globalThis.assert.strictEqual(globalThis.addon.asBool(false), false);
globalThis.assert.strictEqual(globalThis.addon.asBool(true), true);
globalThis.assert.throws(() => globalThis.addon.asBool(undefined));
globalThis.assert.throws(() => globalThis.addon.asBool(null));
globalThis.assert.throws(() => globalThis.addon.asBool(Number.NaN));
globalThis.assert.throws(() => globalThis.addon.asBool(0));
globalThis.assert.throws(() => globalThis.addon.asBool(''));
globalThis.assert.throws(() => globalThis.addon.asBool('0'));
globalThis.assert.throws(() => globalThis.addon.asBool(1));
globalThis.assert.throws(() => globalThis.addon.asBool('1'));
globalThis.assert.throws(() => globalThis.addon.asBool('true'));
globalThis.assert.throws(() => globalThis.addon.asBool({}));
globalThis.assert.throws(() => globalThis.addon.asBool([]));
globalThis.assert.throws(() => globalThis.addon.asBool(testSym));

[globalThis.addon.asInt32, globalThis.addon.asUInt32, globalThis.addon.asInt64].forEach((asInt) => {
  globalThis.assert.strictEqual(asInt(0), 0);
  globalThis.assert.strictEqual(asInt(1), 1);
  globalThis.assert.strictEqual(asInt(1.0), 1);
  globalThis.assert.strictEqual(asInt(1.1), 1);
  globalThis.assert.strictEqual(asInt(1.9), 1);
  globalThis.assert.strictEqual(asInt(0.9), 0);
  globalThis.assert.strictEqual(asInt(999.9), 999);
  // asInt(Number.Nan) 被转换成 int32_t uint32_t int64_t 会损失精度
  // 而 JavaScriptCore Number.NaN 实际上就是一个固定的 double
  //   globalThis.assert.strictEqual(asInt(Number.NaN), 0);
  globalThis.assert.throws(() => asInt(undefined));
  globalThis.assert.throws(() => asInt(null));
  globalThis.assert.throws(() => asInt(false));
  globalThis.assert.throws(() => asInt(''));
  globalThis.assert.throws(() => asInt('1'));
  globalThis.assert.throws(() => asInt({}));
  globalThis.assert.throws(() => asInt([]));
  globalThis.assert.throws(() => asInt(testSym));
});

globalThis.assert.strictEqual(globalThis.addon.asInt32(-1), -1);
globalThis.assert.strictEqual(globalThis.addon.asInt64(-1), -1);
globalThis.assert.strictEqual(globalThis.addon.asUInt32(-1), Math.pow(2, 32) - 1);

globalThis.assert.strictEqual(globalThis.addon.asDouble(0), 0);
globalThis.assert.strictEqual(globalThis.addon.asDouble(1), 1);
globalThis.assert.strictEqual(globalThis.addon.asDouble(1.0), 1.0);
globalThis.assert.strictEqual(globalThis.addon.asDouble(1.1), 1.1);
globalThis.assert.strictEqual(globalThis.addon.asDouble(1.9), 1.9);
globalThis.assert.strictEqual(globalThis.addon.asDouble(0.9), 0.9);
globalThis.assert.strictEqual(globalThis.addon.asDouble(999.9), 999.9);
globalThis.assert.strictEqual(globalThis.addon.asDouble(-1), -1);
globalThis.assert.ok(Number.isNaN(globalThis.addon.asDouble(Number.NaN)));
globalThis.assert.throws(() => globalThis.addon.asDouble(undefined));
globalThis.assert.throws(() => globalThis.addon.asDouble(null));
globalThis.assert.throws(() => globalThis.addon.asDouble(false));
globalThis.assert.throws(() => globalThis.addon.asDouble(''));
globalThis.assert.throws(() => globalThis.addon.asDouble('1'));
globalThis.assert.throws(() => globalThis.addon.asDouble({}));
globalThis.assert.throws(() => globalThis.addon.asDouble([]));
globalThis.assert.throws(() => globalThis.addon.asDouble(testSym));

globalThis.assert.strictEqual(globalThis.addon.asString(''), '');
globalThis.assert.strictEqual(globalThis.addon.asString('test'), 'test');
globalThis.assert.throws(() => globalThis.addon.asString(undefined));
globalThis.assert.throws(() => globalThis.addon.asString(null));
globalThis.assert.throws(() => globalThis.addon.asString(false));
globalThis.assert.throws(() => globalThis.addon.asString(1));
globalThis.assert.throws(() => globalThis.addon.asString(1.1));
globalThis.assert.throws(() => globalThis.addon.asString(Number.NaN));
globalThis.assert.throws(() => globalThis.addon.asString({}));
globalThis.assert.throws(() => globalThis.addon.asString([]));
globalThis.assert.throws(() => globalThis.addon.asString(testSym));

globalThis.assert.strictEqual(globalThis.addon.toBool(true), true);
globalThis.assert.strictEqual(globalThis.addon.toBool(1), true);
globalThis.assert.strictEqual(globalThis.addon.toBool(-1), true);
globalThis.assert.strictEqual(globalThis.addon.toBool('true'), true);
globalThis.assert.strictEqual(globalThis.addon.toBool('false'), true);
globalThis.assert.strictEqual(globalThis.addon.toBool({}), true);
globalThis.assert.strictEqual(globalThis.addon.toBool([]), true);
globalThis.assert.strictEqual(globalThis.addon.toBool(testSym), true);
globalThis.assert.strictEqual(globalThis.addon.toBool(false), false);
globalThis.assert.strictEqual(globalThis.addon.toBool(undefined), false);
globalThis.assert.strictEqual(globalThis.addon.toBool(null), false);
globalThis.assert.strictEqual(globalThis.addon.toBool(0), false);
globalThis.assert.strictEqual(globalThis.addon.toBool(Number.NaN), false);
globalThis.assert.strictEqual(globalThis.addon.toBool(''), false);

globalThis.assert.strictEqual(globalThis.addon.toNumber(0), 0);
globalThis.assert.strictEqual(globalThis.addon.toNumber(1), 1);
globalThis.assert.strictEqual(globalThis.addon.toNumber(1.1), 1.1);
globalThis.assert.strictEqual(globalThis.addon.toNumber(-1), -1);
globalThis.assert.strictEqual(globalThis.addon.toNumber('0'), 0);
globalThis.assert.strictEqual(globalThis.addon.toNumber('1'), 1);
globalThis.assert.strictEqual(globalThis.addon.toNumber('1.1'), 1.1);
globalThis.assert.strictEqual(globalThis.addon.toNumber([]), 0);
globalThis.assert.strictEqual(globalThis.addon.toNumber(false), 0);
globalThis.assert.strictEqual(globalThis.addon.toNumber(null), 0);
globalThis.assert.strictEqual(globalThis.addon.toNumber(''), 0);
globalThis.assert.ok(Number.isNaN(globalThis.addon.toNumber(Number.NaN)));
globalThis.assert.ok(Number.isNaN(globalThis.addon.toNumber({})));
globalThis.assert.ok(Number.isNaN(globalThis.addon.toNumber(undefined)));
globalThis.assert.throws(() => globalThis.addon.toNumber(testSym), TypeError);

// globalThis.assert.deepStrictEqual({}, globalThis.addon.toObject({}));
// globalThis.assert.deepStrictEqual({ test: 1 }, globalThis.addon.toObject({ test: 1 }));
// globalThis.assert.deepStrictEqual([], globalThis.addon.toObject([]));
// globalThis.assert.deepStrictEqual([1, 2, 3], globalThis.addon.toObject([1, 2, 3]));
// globalThis.assert.deepStrictEqual(new Boolean(false), globalThis.addon.toObject(false));
// globalThis.assert.deepStrictEqual(new Boolean(true), globalThis.addon.toObject(true));
// globalThis.assert.deepStrictEqual(new String(''), globalThis.addon.toObject(''));
// globalThis.assert.deepStrictEqual(new Number(0), globalThis.addon.toObject(0));
// globalThis.assert.deepStrictEqual(new Number(Number.NaN), globalThis.addon.toObject(Number.NaN));
// globalThis.assert.deepStrictEqual(new Object(testSym), globalThis.addon.toObject(testSym));
// globalThis.assert.notDeepStrictEqual(globalThis.addon.toObject(false), false);
// globalThis.assert.notDeepStrictEqual(globalThis.addon.toObject(true), true);
// globalThis.assert.notDeepStrictEqual(globalThis.addon.toObject(''), '');
// globalThis.assert.notDeepStrictEqual(globalThis.addon.toObject(0), 0);
globalThis.assert.ok(!Number.isNaN(globalThis.addon.toObject(Number.NaN)));

globalThis.assert.strictEqual(globalThis.addon.toString(''), '');
globalThis.assert.strictEqual(globalThis.addon.toString('test'), 'test');
globalThis.assert.strictEqual(globalThis.addon.toString(undefined), 'undefined');
globalThis.assert.strictEqual(globalThis.addon.toString(null), 'null');
globalThis.assert.strictEqual(globalThis.addon.toString(false), 'false');
globalThis.assert.strictEqual(globalThis.addon.toString(true), 'true');
globalThis.assert.strictEqual(globalThis.addon.toString(0), '0');
globalThis.assert.strictEqual(globalThis.addon.toString(1.1), '1.1');
globalThis.assert.strictEqual(globalThis.addon.toString(Number.NaN), 'NaN');
globalThis.assert.strictEqual(globalThis.addon.toString({}), '[object Object]');
globalThis.assert.strictEqual(globalThis.addon.toString({ toString: () => 'test' }), 'test');
globalThis.assert.strictEqual(globalThis.addon.toString([]), '');
globalThis.assert.strictEqual(globalThis.addon.toString([1, 2, 3]), '1,2,3');
globalThis.assert.throws(() => globalThis.addon.toString(testSym));

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueBool(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'A boolean was expected'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueInt32(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'A number was expected'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueUint32(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'A number was expected'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueInt64(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'A number was expected'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueDouble(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'A number was expected'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.coerceToBool(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'napi_ok'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.coerceToObject(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'napi_ok'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.coerceToString(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument',
//   inputTypeCheck: 'napi_ok'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueStringUtf8(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   wrongTypeIn: 'A string was expected',
//   bufAndOutLengthIsNull: 'Invalid argument'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueStringLatin1(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   wrongTypeIn: 'A string was expected',
//   bufAndOutLengthIsNull: 'Invalid argument'
// });

// globalThis.assert.deepStrictEqual(globalThis.addon.testNull.getValueStringUtf16(), {
//   envIsNull: 'Invalid argument',
//   valueIsNull: 'Invalid argument',
//   wrongTypeIn: 'A string was expected',
//   bufAndOutLengthIsNull: 'Invalid argument'
// });

export { }