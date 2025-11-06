package debug

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/ParthGandhiNUS/CG4002/internal/types"
)

var (
	DebugCollector    *DataCollector
	mu                sync.RWMutex
	SendDebugCallback func(data interface{})
)

var DURATION = 5 * time.Second

func RegisterSendDebugCallback(cb func(data interface{})) {
	SendDebugCallback = cb
}

type DataCollector struct {
	data     map[types.TimeField]int64
	required []types.TimeField
	dataLock sync.RWMutex
	ctx      context.Context
	cancel   context.CancelFunc
	done     chan struct{}
}

func IsDebugActive() bool {
	mu.RLock()
	defer mu.RUnlock()
	return DebugCollector != nil
}

func GetDebugCollector() *DataCollector {
	mu.RLock()
	defer mu.RUnlock()
	return DebugCollector
}

func StartDebugSession() bool {
	mu.Lock()
	defer mu.Unlock()

	if IsDebugActive() {
		return false
	}

	ctx, cancel := context.WithTimeout(context.Background(), DURATION)

	DebugCollector = &DataCollector{
		data: make(map[types.TimeField]int64),
		required: []types.TimeField{
			types.ORIGINAL_TIME,
			types.ESP32_TO_SERVER,
			types.ESP32_TO_ULTRA96,
			types.ULTRA96_TO_SERVER,
			types.INFERENCE_TIME,
			types.SERVER_TO_VIS,
			types.END_TO_END_GESTURE,
			types.END_TO_END_VOICE,
		},
		ctx:    ctx,
		cancel: cancel,
		done:   make(chan struct{}),
	}

	go run()
	return true
}

func (dc *DataCollector) AddData(key types.TimeField, value int64) bool {
	dc.dataLock.Lock()
	defer dc.dataLock.Unlock()

	if dc.ctx.Err() != nil {
		return false
	}

	dc.data[key] = value

	if dc.isComplete() {
		dc.cancel()
	}
	return true
}

func AddData(key types.TimeField, value int64) bool {
	collector := GetDebugCollector()
	if collector == nil {
		return false
	}
	return collector.AddData(key, value)
}

// Make a thread-safe copy of data map
func (dc *DataCollector) getDataCopy() map[types.TimeField]int64 {
	dc.dataLock.RLock()
	defer dc.dataLock.RUnlock()

	copy := make(map[types.TimeField]int64, len(dc.data))
	for k, v := range dc.data {
		copy[k] = v
	}
	return copy
}

func (dc *DataCollector) isComplete() bool {
	dc.dataLock.RLock()
	defer dc.dataLock.RUnlock()

	for _, key := range dc.required {
		if _, exists := dc.data[key]; !exists {
			return false
		}
	}
	return true
}

func toLatencyInfo(data map[types.TimeField]int64) *types.LatencyInfo {
	fmtLatency := func(t types.TimeField) string {
		v, ok := data[t]
		if !ok {
			return ""
		}
		return fmt.Sprintf("%dms", v)
	}

	return &types.LatencyInfo{
		ESP32ToServer:      fmtLatency(types.ESP32_TO_SERVER),
		ESP32ToUltra96:     fmtLatency(types.ESP32_TO_ULTRA96),
		Ultra96ToServer:    fmtLatency(types.ULTRA96_TO_SERVER),
		ServerToVisualiser: fmtLatency(types.SERVER_TO_VIS),
		InferenceTime:      fmtLatency(types.INFERENCE_TIME),
		EndToEndGesture:    fmtLatency(types.END_TO_END_GESTURE),
		EndToEndVoice:      fmtLatency(types.END_TO_END_VOICE),
	}
}

func run() {
	<-DebugCollector.ctx.Done()

	mu.Lock()
	defer mu.Unlock()

	if DebugCollector.isComplete() {
		dataCopy := DebugCollector.getDataCopy()
		latencyInfo := toLatencyInfo(dataCopy)
		if SendDebugCallback != nil {
			SendDebugCallback(latencyInfo)
		}
	}
	close(DebugCollector.done)
	DebugCollector = nil
}

func Wait() {
	dc := GetDebugCollector()
	if dc == nil {
		return
	}
	<-dc.done
}
