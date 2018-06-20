#ifndef _ADAFRUIT_ZERODMA_H_
#define _ADAFRUIT_ZERODMA_H_

#include "Arduino.h"
#include "utility/dma.h"

// Status codes returned by some DMA functions and/or held in
// a channel's jobStatus variable.
enum ZeroDMAstatus {
	DMA_STATUS_OK = 0,
	DMA_STATUS_ERR_NOT_FOUND,
	DMA_STATUS_ERR_NOT_INITIALIZED,
	DMA_STATUS_ERR_INVALID_ARG,
	DMA_STATUS_ERR_IO,
	DMA_STATUS_ERR_TIMEOUT,
	DMA_STATUS_BUSY,
	DMA_STATUS_SUSPEND,
	DMA_STATUS_ABORTED,
	DMA_STATUS_JOBSTATUS = -1 // For printStatus() function
};

class Adafruit_ZeroDMA {
 public:
  Adafruit_ZeroDMA(void);

  // DMA channel functions
  ZeroDMAstatus   allocate(void), // Allocates DMA channel
                  startJob(void),
                  free(void);     // Deallocates DMA channel
  void            trigger(void),
                  setTrigger(uint8_t trigger),
                  setAction(dma_transfer_trigger_action action),
                  setCallback(void (*callback)(void *) = NULL,
                    dma_callback_type type = DMA_CALLBACK_TRANSFER_DONE,
                    void *data = NULL),
                  loop(boolean flag),
                  suspend(void),
                  resume(void),
                  abort(void);
                  //printStatus(ZeroDMAstatus s = DMA_STATUS_JOBSTATUS);

  // DMA descriptor functions
  DmacDescriptor *addDescriptor(void *src, void *dst, uint32_t count = 0,
                    dma_beat_size size = DMA_BEAT_SIZE_BYTE,
                    bool srcInc = true, bool dstInc = true,
					uint32_t stepSize = DMA_ADDRESS_INCREMENT_STEP_SIZE_1,
					bool stepSel = DMA_STEPSEL_DST);
  void            changeDescriptor(DmacDescriptor *d, void *src = NULL,
                    void *dst = NULL, uint32_t count = 0);

  void            _IRQhandler(uint8_t flags); // DO NOT TOUCH

 protected:
  uint8_t                     channel;
  volatile enum ZeroDMAstatus jobStatus;
  bool                        hasDescriptors;
  bool                        loopFlag;
  uint8_t                     peripheralTrigger;
  dma_transfer_trigger_action triggerAction;
  void                      (*callback[DMA_CALLBACK_N])(void *);
  void                       *callbackData[DMA_CALLBACK_N];
};

#endif // _ADAFRUIT_ZERODMA_H_
