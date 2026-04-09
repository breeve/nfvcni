package controlplane

import (
	"github.com/breeve/nfvcni/test/e2e/framework"

	"github.com/onsi/ginkgo/v2"
	"github.com/onsi/gomega"
)

var _ = ginkgo.Describe("ControlPlane Health", func() {
	var cfg *framework.Config
	var client *framework.HTTPClient

	ginkgo.BeforeEach(func() {
		cfg = framework.GetConfig()
		client = framework.NewHTTPClient(cfg)
	})

	ginkgo.Describe("GET /health", func() {
		ginkgo.It("should return 200 OK", func() {
			resp, err := client.Get("/health")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			gomega.Expect(resp.StatusCode).To(gomega.Equal(200))
			resp.Body.Close()
		})

		ginkgo.It("should return JSON", func() {
			resp, err := client.Get("/health")
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
			contentType := resp.Header.Get("Content-Type")
			gomega.Expect(contentType).To(gomega.ContainSubstring("application/json"))
			resp.Body.Close()
		})
	})
})