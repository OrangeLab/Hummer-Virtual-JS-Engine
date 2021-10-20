//
// Created by didi on 2021/10/19.
//

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include <hermes/hermes.h>
#include <hermes/inspector/RuntimeAdapter.h>

namespace orangelab
{
namespace hermes
{
namespace inspector
{
namespace chrome
{
/**
 * 由于 hermes debugger 所有调试功能需要在 调试器 已经链接的情况生效，
 * 如： waitingForDebugger ， pauseOnLoad 等
 *
 * 因此 为保证 breakpoint 生效足够早，因此暴露 waitForDebugger 参数。
 *
 * */

void enableDebugging(
    std::unique_ptr<facebook::hermes::inspector::RuntimeAdapter> adapter,
    const std::string &title,
    bool waitForDebugger);


void disableDebugging(facebook::hermes::HermesRuntime &runtime);

} // namespace chrome
} // namespace inspector
} // namespace hermes
} // namespace orangelab

