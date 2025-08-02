/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/fs/vfs.h>

/*
 * Mount a new tmpfs drive.
 * Returns the drive ID when successful, or -1 on error.
 */
int tmpfs_new_drive();
