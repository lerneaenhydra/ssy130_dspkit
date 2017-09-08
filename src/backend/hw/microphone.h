/**
 * @file         microphone.h
 * @version      1.0
 * @date         2015
 * @author       Christoph Lauer
 * @compiler     armcc
 * @copyright    Christoph Lauer engineering
 */
 
#ifndef __MICROPHONE_H
#define __MICROPHONE_H

#include <stdbool.h>
#include "stm32f4xx.h"

/** @brief Triggers microphone sampling */
void WaveRecorderBeginSampling(void);

/** @brief Triggers filting any buffered PDM data */
void WaveRecorderPDMFiltCallback(void);

#endif
