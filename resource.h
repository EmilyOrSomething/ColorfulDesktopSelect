#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IDD_MAIN    100
#define IDC_ACCENT  101
#define IDC_COLOR   102
#define IDC_HIDE    201
#define IDC_APPLY   202

#define APP_NAME    L"ColorfulDesktopSelect"
#define IDI_APP_ICON 1000

#define WM_APP_VIEW (WM_USER + 0)

const BYTE pattern1[] = {
    0x0F, 0x1F, 0x44, 0x00, 0x00,               // nop dword ptr [rax+rax]
    0x41, 0x8D, 0x4D, 0x19,                     // lea ecx, [r13+19h]
    0x4C, 0x8B, 0xF8,                           // mov r15, rax
    0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // call qword ptr [comctl32!__imp_GetSysColor]
    0x0F, 0x1F, 0x44, 0x00, 0x00,               // nop dword ptr [rax+rax]
    0x48, 0x8D, 0x55, 0xC7,                     // lea rdx, [rbp-39h]
    0x48, 0x8B, 0xCB                            // mov rcx, rbx
};

const BYTE pattern2[] = {
    0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // call qword ptr [comctl32!__imp_GetSysColor]
    0x0F, 0x1F, 0x44, 0x00, 0x00,               // nop dword ptr [rax+rax]
    0xB9, 0x0D, 0x00, 0x00, 0x00,               // mov ecx, 0Dh
    0x8B, 0xD8,                                 // mov ebx, eax
    0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // call qword ptr [comctl32!__imp_GetSysColor]
    0x0F, 0x1F, 0x44, 0x00, 0x00,               // nop dword ptr [rax+rax]
    0x48, 0x8B, 0x57, 0x18,                     // mov rdx, qword ptr [rdi+18h]
    0x44, 0x8b, 0xcb                            // mov r9d, ebx
};

const char mask1[] = "xxxxxxxxxxxxx???????????xxxxxxx";
const char mask2[] = "x???????????xxxxxxxx???????????xxxxxxx";
