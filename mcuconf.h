// Copyright 2023 Cole Smith (@boardsource)
// Copyright 2024 mass
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include_next <mcuconf.h>

#undef RP_SPI_USE_SPI0
#define RP_SPI_USE_SPI0 TRUE

#undef RP_I2C_USE_I2C0
#define RP_I2C_USE_I2C0 TRUE

#undef RP_ADC_USE_ADC0
#define RP_ADC_USE_ADC0 TRUE

#undef RP_ADC_USE_ADC1
#define RP_ADC_USE_ADC1 TRUE

#undef RP_ADC_USE_ADC2
#define RP_ADC_USE_ADC2 TRUE

#undef RP_ADC_USE_ADC3
#define RP_ADC_USE_ADC3 TRUE
