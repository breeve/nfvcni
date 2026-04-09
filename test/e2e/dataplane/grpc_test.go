package dataplane

import (
	"context"
	"time"

	"github.com/breeve/nfvcni/pkg/dataplane/server/proto"
	"github.com/breeve/nfvcni/test/e2e/framework"

	"github.com/onsi/ginkgo/v2"
	"github.com/onsi/gomega"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

var _ = ginkgo.Describe("Dataplane gRPC", func() {
	var cfg *framework.Config
	var conn *grpc.ClientConn
	var client proto.DataplaneServiceClient
	var ctx context.Context

	ginkgo.BeforeEach(func() {
		cfg = framework.GetConfig()
		ctx, _ = context.WithTimeout(context.Background(), 10*time.Second)

		var err error
		conn, err = grpc.DialContext(
			ctx,
			cfg.DataplaneAddr(),
			grpc.WithTransportCredentials(insecure.NewCredentials()),
			grpc.WithBlock(),
		)
		gomega.Expect(err).NotTo(gomega.HaveOccurred())
		client = proto.NewDataplaneServiceClient(conn)
	})

	ginkgo.AfterEach(func() {
		if conn != nil {
			conn.Close()
		}
	})

	ginkgo.Describe("AttachPort", func() {
		ginkgo.It("should attach a port", func() {
			req := &proto.AttachPortRequest{
				Ifindex: 100,
			}
			_, err := client.AttachPort(ctx, req)
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
		})
	})

	ginkgo.Describe("DetachPort", func() {
		ginkgo.It("should detach a port", func() {
			req := &proto.DetachPortRequest{
				Ifindex: 100,
			}
			_, err := client.DetachPort(ctx, req)
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
		})
	})

	ginkgo.Describe("GetFdbTable", func() {
		ginkgo.It("should return FDB table", func() {
			req := &proto.GetFdbTableRequest{}
			_, err := client.GetFdbTable(ctx, req)
			gomega.Expect(err).NotTo(gomega.HaveOccurred())
		})
	})
})