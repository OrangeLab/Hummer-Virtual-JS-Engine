const empty = '';
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1(empty), empty);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(empty), empty);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(empty), empty);
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(empty), 0);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(empty), 0);

const str1 = 'hello world';
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1(str1), str1);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(str1), str1);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(str1), str1);
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1Insufficient(str1), str1.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(str1), str1.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(str1), str1.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(str1), 11);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(str1), 11);

const str2 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1(str2), str2);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(str2), str2);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(str2), str2);
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1Insufficient(str2), str2.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(str2), str2.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(str2), str2.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(str2), 62);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(str2), 62);

const str3 = '?!@#$%^&*()_+-=[]{}/.,<>\'"\\';
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1(str3), str3);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(str3), str3);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(str3), str3);
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1Insufficient(str3), str3.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(str3), str3.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(str3), str3.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(str3), 27);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(str3), 27);

const str4 = '¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿';
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1(str4), str4);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(str4), str4);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(str4), str4);
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1Insufficient(str4), str4.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(str4), str4.slice(0, 1));
globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(str4), str4.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(str4), 31);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(str4), 62);

const str5 = 'ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþ';
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1(str5), str5);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(str5), str5);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(str5), str5);
// globalThis.assert.strictEqual(globalThis.addon.TestLatin1Insufficient(str5), str5.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(str5), str5.slice(0, 1));
globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(str5), str5.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(str5), 63);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(str5), 126);

const str6 = '\u{2003}\u{2101}\u{2001}\u{202}\u{2011}';
globalThis.assert.strictEqual(globalThis.addon.TestUtf8(str6), str6);
globalThis.assert.strictEqual(globalThis.addon.TestUtf16(str6), str6);
globalThis.assert.strictEqual(globalThis.addon.TestUtf8Insufficient(str6), str6.slice(0, 1));
globalThis.assert.strictEqual(globalThis.addon.TestUtf16Insufficient(str6), str6.slice(0, 3));
globalThis.assert.strictEqual(globalThis.addon.Utf16Length(str6), 5);
globalThis.assert.strictEqual(globalThis.addon.Utf8Length(str6), 14);

// globalThis.assert.throws(() => {
//   globalThis.addon.TestLargeUtf8();
// }, /^Error: Invalid argument$/);

// globalThis.assert.throws(() => {
//   globalThis.addon.TestLargeLatin1();
// }, /^Error: Invalid argument$/);

// globalThis.assert.throws(() => {
//   globalThis.addon.TestLargeUtf16();
// }, /^Error: Invalid argument$/);

globalThis.addon.TestMemoryCorruption(' '.repeat(64 * 1024));

export { }