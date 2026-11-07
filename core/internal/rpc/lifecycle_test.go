package rpc

import (
	"context"
	"os"
	"testing"
	"time"

	"ThroneCore/gen"
)

func TestExtraProcessHelper(t *testing.T) {
	if os.Getenv("GO_WANT_EXTRA_PROCESS_HELPER") != "1" {
		return
	}
	for {
		time.Sleep(time.Hour)
	}
}

func TestStartFailureStopsExtraProcess(t *testing.T) {
	t.Cleanup(func() {
		if extraProcess != nil {
			extraProcess.Stop()
			extraProcess = nil
		}
		closeXray()
		setBoxInstance(nil, nil, nil)
		autoRedirectMark.Store(0)
	})

	t.Setenv("GO_WANT_EXTRA_PROCESS_HELPER", "1")
	executable, err := os.Executable()
	if err != nil {
		t.Fatal(err)
	}
	coreConfig := `{"outbounds":[{"type":"direct","tag":"direct"}]}`
	xrayConfig := `{`
	response, err := (&server{}).Start(context.Background(), &gen.LoadConfigReq{
		CoreConfig:       &coreConfig,
		NeedExtraProcess: To(true),
		ExtraProcessPath: To(executable),
		ExtraProcessArgs: To("-test.run=^TestExtraProcessHelper$"),
		ExtraNoOut:       To(true),
		NeedXray:         To(true),
		XrayConfig:       &xrayConfig,
	})
	if err != nil {
		t.Fatal(err)
	}
	if response.Error == nil {
		t.Fatal("Start accepted a malformed Xray config")
	}
	if extraProcess != nil {
		t.Fatal("failed Start retained the running extra process")
	}

	if _, err = (&server{}).Stop(context.Background(), &gen.EmptyReq{}); err != nil {
		t.Fatal(err)
	}
}

func TestDuplicateStartKeepsRunningExtraProcess(t *testing.T) {
	t.Cleanup(func() {
		_, _ = (&server{}).Stop(context.Background(), &gen.EmptyReq{})
		if extraProcess != nil {
			extraProcess.Stop()
			extraProcess = nil
		}
	})

	t.Setenv("GO_WANT_EXTRA_PROCESS_HELPER", "1")
	executable, err := os.Executable()
	if err != nil {
		t.Fatal(err)
	}
	coreConfig := `{"outbounds":[{"type":"direct","tag":"direct"}]}`
	request := &gen.LoadConfigReq{
		CoreConfig:       &coreConfig,
		NeedExtraProcess: To(true),
		ExtraProcessPath: To(executable),
		ExtraProcessArgs: To("-test.run=^TestExtraProcessHelper$"),
		ExtraNoOut:       To(true),
		NeedXray:         To(false),
	}
	response, err := (&server{}).Start(context.Background(), request)
	if err != nil {
		t.Fatal(err)
	}
	if response.Error != nil {
		t.Fatalf("first Start failed: %s", *response.Error)
	}
	startedProcess := extraProcess
	if startedProcess == nil {
		t.Fatal("first Start did not retain its extra process")
	}
	startedBox := currentBox()
	if startedBox == nil {
		t.Fatal("first Start did not retain its box")
	}

	response, err = (&server{}).Start(context.Background(), request)
	if err != nil {
		t.Fatal(err)
	}
	if response.Error == nil {
		t.Fatal("duplicate Start unexpectedly succeeded")
	}
	if extraProcess != startedProcess {
		t.Fatal("duplicate Start stopped the running profile's extra process")
	}
	if currentBox() != startedBox {
		t.Fatal("duplicate Start discarded the running profile's box")
	}
	if _, err = (&server{}).Stop(context.Background(), &gen.EmptyReq{}); err != nil {
		t.Fatal(err)
	}
	if currentBox() != nil || extraProcess != nil {
		t.Fatal("Stop did not tear down the original profile after duplicate Start")
	}
}
