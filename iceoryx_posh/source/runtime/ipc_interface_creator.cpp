// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx_posh/internal/runtime/ipc_interface_creator.hpp"
#include "iceoryx_posh/internal/posh_error_reporting.hpp"
#include "iox/filesystem.hpp"

namespace iox
{
namespace runtime
{
IpcInterfaceCreator::IpcInterfaceCreator(const RuntimeName_t& runtimeName,
                                         const ResourceType resourceType,
                                         const uint64_t maxMessages,
                                         const uint64_t messageSize) noexcept
    : IpcInterfaceBase(runtimeName, resourceType, maxMessages, messageSize)
    , m_fileLock(std::move(FileLockBuilder()
                               .name(m_interfaceName)
                               .permission(iox::perms::owner_read | iox::perms::owner_write)
                               .create()
                               .or_else([this](auto& error) {
                                   if (error == FileLockError::LOCKED_BY_OTHER_PROCESS)
                                   {
                                       IOX_LOG(FATAL,
                                               "An application with the name " << m_runtimeName
                                                                               << " is still running. Using the "
                                                                                  "same name twice is not supported.");
                                       IOX_REPORT_FATAL(PoshError::IPC_INTERFACE__APP_WITH_SAME_NAME_STILL_RUNNING);
                                   }
                                   else
                                   {
                                       IOX_LOG(FATAL,
                                               "Error occurred while acquiring file lock named " << m_interfaceName);
                                       IOX_REPORT_FATAL(PoshError::IPC_INTERFACE__COULD_NOT_ACQUIRE_FILE_LOCK);
                                   }
                               })
                               .value()))
{
    // check if the IPC channel is still there (e.g. because of no proper termination
    // of the process)
    cleanupOutdatedIpcChannel(m_interfaceName);

    openIpcChannel(PosixIpcChannelSide::SERVER);
}
} // namespace runtime
} // namespace iox
