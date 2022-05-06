/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Janik Schlemminger
 *    Matthias Jung
 */

#ifndef DEBUGMANAGER_H
#define DEBUGMANAGER_H

#if __has_cpp_attribute(maybe_unused)
    #define NDEBUG_UNUSED(var_decl) [[maybe_unused]] var_decl
#elif (__GNUC__ || __clang__)
    #define NDEBUG_UNUSED(var_decl) var_decl __attribute__((unused))
#else
    #define	NDEBUG_UNUSED(var_decl) var_decl
#endif

#ifdef NDEBUG
#define PRINTDEBUGMESSAGE(sender, message) {}
#else
#define PRINTDEBUGMESSAGE(sender, message) DebugManager::getInstance().printDebugMessage(sender, message)

#include <string>
#include <fstream>

class DebugManager
{
public:
    static DebugManager &getInstance()
    {
        static DebugManager _instance;
        return _instance;
    }
    ~DebugManager();

private:
    DebugManager();
    DebugManager(const DebugManager &);
    DebugManager & operator = (const DebugManager &);

public:
    void setup(bool _debugEnabled, bool _writeToConsole, bool _writeToFile);

    void printDebugMessage(const std::string &sender, const std::string &message);
    static void printMessage(const std::string &sender, const std::string &message);
    void openDebugFile(const std::string &filename);

private:
    bool debugEnabled;
    bool writeToConsole;
    bool writeToFile;

    std::ofstream debugFile;
};
#endif

#endif // DEBUGMANAGER_H
