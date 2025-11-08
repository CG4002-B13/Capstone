package debug

import (
	"context"
	"fmt"
	"log"
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

// FIX 1: Protect with mutex
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

	log.Println("Acquired lock in StartDebugSession")

	if DebugCollector != nil {
		log.Println("Debug is already active, returning")
		return false
	}

	ctx, cancel := context.WithTimeout(context.Background(), DURATION)
	DebugCollector = &DataCollector{
		data: make(map[types.TimeField]int64),
		required: []types.TimeField{
			types.INITIAL_MQTT_TIME,
			types.ESP32_TO_SERVER,
			types.ESP32_TO_ULTRA96,
			types.ULTRA96_TO_SERVER,
			types.INFERENCE_TIME,
			types.INITIAL_SERVER_TIME,
			types.SERVER_TO_VIS,
			types.END_TO_END_GESTURE,
			types.END_TO_END_VOICE,
		},
		ctx:    ctx,
		cancel: cancel,
		done:   make(chan struct{}),
	}

	log.Printf("DebugCollector created and started")
	go DebugCollector.run() // FIX 3: Call as method with receiver
	return true
}

// FIX 2: Create internal version without lock
func (dc *DataCollector) isCompleteLocked() bool {
	for _, key := range dc.required {
		if _, exists := dc.data[key]; !exists {
			return false
		}
	}
	return true
}

func (dc *DataCollector) AddData(key types.TimeField, value int64) bool {
	dc.dataLock.Lock()
	defer dc.dataLock.Unlock()

	if dc.ctx.Err() != nil {
		return false
	}

	dc.data[key] = value
	log.Printf("Added %d to %s", value, key)

	// FIX 2: Use internal version that doesn't acquire lock
	if dc.isCompleteLocked() {
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

func GetData(key types.TimeField) int64 {
	collector := GetDebugCollector()
	if collector == nil {
		return 0
	}
	return collector.GetData(key)
}

func (dc *DataCollector) GetData(key types.TimeField) int64 {
	dc.dataLock.RLock()
	defer dc.dataLock.RUnlock()
	return dc.data[key]
}

// Make a thread-safe copy of data map
func (dc *DataCollector) getFullDataCopy() map[types.TimeField]int64 {
	dc.dataLock.RLock()
	defer dc.dataLock.RUnlock()

	copy := make(map[types.TimeField]int64, len(dc.data))
	for k, v := range dc.data {
		copy[k] = v
	}
	return copy
}

// Public version with lock (if needed elsewhere)
func (dc *DataCollector) isComplete() bool {
	dc.dataLock.RLock()
	defer dc.dataLock.RUnlock()
	return dc.isCompleteLocked()
}

func toLatencyInfo(data map[types.TimeField]int64) *types.LatencyInfo {
	fmtLatency := func(t types.TimeField) string {
		v, ok := data[t]
		if !ok {
			return "no data available"
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

// FIX 3: Make this a method so it has proper receiver
func (dc *DataCollector) run() {
	<-dc.ctx.Done()

	// Get complete status and data copy
	dataCopy := dc.getFullDataCopy()
	latencyInfo := toLatencyInfo(dataCopy)

	// Now acquire global mutex to clear DebugCollector
	mu.Lock()
	DebugCollector = nil
	mu.Unlock()

	// Send callback after releasing mutex to avoid holding lock during callback
	if SendDebugCallback != nil {
		SendDebugCallback(latencyInfo)
	}

	close(dc.done)
}

func Wait() {
	dc := GetDebugCollector()
	if dc == nil {
		return
	}
	<-dc.done
}
