// Copyright (C) 2015 The Regents of the University of California (Regents).
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents or University of California nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Steffen Urban, urbste@gmail.com

#include <Eigen/Core>
#include <Eigen/LU>
#include <algorithm>
#include <ceres/rotation.h>
#include <glog/logging.h>

#include "theia/matching/feature_correspondence.h"
#include "theia/math/util.h"
#include "theia/sfm/bundle_adjustment/bundle_adjustment.h"
#include "theia/sfm/camera/camera.h"
#include "theia/sfm/pose/test_util.h"
#include "theia/sfm/reconstruction.h"
#include "theia/util/random.h"
#include "gtest/gtest.h"

namespace theia {

namespace {
RandomNumberGenerator rng(52);

Camera RandomCamera() {
  Camera camera;
  camera.SetPosition(rng.RandVector3d());
  camera.SetOrientationFromAngleAxis(0.2 * rng.RandVector3d());
  camera.SetImageSize(1000, 1000);
  camera.SetFocalLength(500);
  camera.SetPrincipalPoint(500, 500);
  return camera;
}

void TestOptimizeView(const int kNumPoints, const double kPixelNoise) {
  // Set up random cameras.
  Camera camera1 = RandomCamera();
  Reconstruction reconstruction;
  ViewId vid = reconstruction.AddView("0", 0, 0.0);
  reconstruction.MutableView(vid)->MutableCamera()->DeepCopy(camera1);
  reconstruction.MutableView(vid)->SetEstimated(true);
  // Set up random points.
  for (int i = 0; i < kNumPoints; i++) {
    Eigen::Vector3d point(rng.RandDouble(-5.0, 5.0),
                          rng.RandDouble(-5.0, 5.0),
                          rng.RandDouble(4.0, 10.0));
    TrackId tid = reconstruction.AddTrack();
    reconstruction.MutableTrack(tid)->SetPoint(point.homogeneous());
    reconstruction.MutableTrack(tid)->SetEstimated(true);

    Eigen::Vector2d pixel;
    double depth = reconstruction.View(vid)->Camera().ProjectPoint(
        point.homogeneous(), &pixel);
    if (kPixelNoise > 0.0) {
      AddNoiseToProjection(kPixelNoise, &rng, &pixel);
    }
    if (depth > 0.0) {
      reconstruction.AddObservation(vid, tid, Feature(pixel));
    }
  }

  BundleAdjustmentOptions opts;
  opts.verbose = true;
  BundleAdjustmentSummary sum = BundleAdjustView(opts, vid, &reconstruction);
  std::cout << "Success: " << sum.success << "\n";
  std::cout << "Final squared reprojection error: " << 2.0 * sum.final_cost
            << "\n";
  if (kPixelNoise == 0.0) {
    EXPECT_TRUE(2.0 * sum.final_cost / reconstruction.View(vid)->NumFeatures() <
                1e-15);
  } else {
    EXPECT_TRUE(2.0 * sum.final_cost / reconstruction.View(vid)->NumFeatures() <
                kPixelNoise);
  }
}

}  // namespace

TEST(OptimizeView, NoNoise) {
  static const double kPixelNoise = 0.0;
  static const int kNumPoints = 100;
  TestOptimizeView(kNumPoints, kPixelNoise);
}

TEST(OptimizeView, Noise) {
  static const double kPixelNoise = 0.1;
  static const int kNumPoints = 100;
  TestOptimizeView(kNumPoints, kPixelNoise);
}

}  // namespace theia
