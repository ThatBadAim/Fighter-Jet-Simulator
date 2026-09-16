#include "fastjet/graphics/camera_rig.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace fastjet::graphics;
using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::flcs;

void test_dep_and_equilibrium() {
    std::cout << "[Test] Camera Rig: Design Eye Point (DEP) & 1G Equilibrium... ";

    CameraRig rig;
    const Vector3 eye = rig.eye_pos_body();

    // Verify nominal DEP
    assert(std::abs(eye.x - 1.80) < 1e-4);
    assert(std::abs(eye.y - 0.00) < 1e-4);
    assert(std::abs(eye.z - (-0.65)) < 1e-4);

    // Simulate 2 seconds of 1G wings-level trim (Nz = 1.0, Ny = 0, Nx = 0)
    IMUData imu_1g{};
    imu_1g.Nz = 1.0;
    imu_1g.Ny = 0.0;
    imu_1g.Nx = 0.0;

    for (int i = 0; i < 400; ++i) { // 200 Hz for 2.0 seconds
        rig.update(0.005, imu_1g);
    }

    const Vector3 disp_1g = rig.head_displacement();
    assert(std::abs(disp_1g.x) < 1e-4);
    assert(std::abs(disp_1g.y) < 1e-4);
    assert(std::abs(disp_1g.z) < 1e-4);

    std::cout << "PASSED (DEP = [1.80, 0.00, -0.65] m, zero 1G displacement)\n";
}

void test_g_load_dynamics() {
    std::cout << "[Test] Camera Rig: G-Load Spring-Damper Head Dynamics... ";

    CameraRig rig;

    // 1. High-G Pull: Nz = +9.0 G
    IMUData imu_9g{};
    imu_9g.Nz = 9.0;
    imu_9g.Ny = 0.0;
    imu_9g.Nx = 0.0;

    for (int i = 0; i < 200; ++i) { // 1.0s to settle
        rig.update(0.005, imu_9g);
    }

    Vector3 disp_9g = rig.head_displacement();
    // In aircraft body coords, +Z is down. Pilot pushed into seat: disp_z > 0
    assert(disp_9g.z > 0.025); // At least 2.5 cm downward
    assert(disp_9g.z <= CameraRig::LIMIT_Z_MAX + 1e-4); // Within limit (6 cm)

    // 2. Pushover: Nz = -3.0 G
    IMUData imu_neg3g{};
    imu_neg3g.Nz = -3.0;
    for (int i = 0; i < 200; ++i) {
        rig.update(0.005, imu_neg3g);
    }

    Vector3 disp_neg3g = rig.head_displacement();
    // Pilot lifted towards canopy: disp_z < 0
    assert(disp_neg3g.z < -0.010);
    assert(disp_neg3g.z >= CameraRig::LIMIT_Z_MIN - 1e-4);

    // 3. Lateral G: Ny = +2.0 G (sideforce right -> head pushed left, -Y)
    IMUData imu_lateral{};
    imu_lateral.Nz = 1.0;
    imu_lateral.Ny = 2.0;
    for (int i = 0; i < 200; ++i) {
        rig.update(0.005, imu_lateral);
    }

    Vector3 disp_lat = rig.head_displacement();
    assert(disp_lat.y < -0.015); // Head pushed left

    // 4. Return to neutral upon release (Nz = 1.0, Ny = 0.0)
    IMUData imu_release{};
    imu_release.Nz = 1.0;
    for (int i = 0; i < 400; ++i) {
        rig.update(0.005, imu_release);
    }

    Vector3 disp_rel = rig.head_displacement();
    assert(std::abs(disp_rel.x) < 1e-3);
    assert(std::abs(disp_rel.y) < 1e-3);
    assert(std::abs(disp_rel.z) < 1e-3);

    std::cout << "PASSED (+9G: +" << disp_9g.z * 100.0 << " cm down, -3G: "
              << disp_neg3g.z * 100.0 << " cm up, returned to 0 cm)\n";
}

void test_view_matrices() {
    std::cout << "[Test] Camera Rig: View Matrices & Transformations... ";

    CameraRig rig;
    FlightState state{};
    state.pos_ned = Vector3(0.0, 0.0, -1000.0); // 1000 m altitude
    state.vel_b   = Vector3(200.0, 0.0, 0.0);
    state.q_att   = Quaternion::identity();

    Mat4 v_world   = rig.compute_world_view_matrix(state);
    Mat4 v_cockpit = rig.compute_cockpit_view_matrix();
    Mat4 v_inf     = rig.compute_infinity_view_matrix(state);

    // In level flight:
    // View world: X_cam = Body Y, Y_cam = Body -Z (Up), Z_cam = Body -X (Backwards)
    // Row 0 = [0, 1, 0]
    assert(std::abs(v_world(0, 1) - 1.0f) < 1e-4f);
    // Row 1 = [0, 0, -1]
    assert(std::abs(v_world(1, 2) - (-1.0f)) < 1e-4f);
    // Row 2 = [-1, 0, 0]
    assert(std::abs(v_world(2, 0) - (-1.0f)) < 1e-4f);

    // Cockpit view matrix translation: -eye_body
    // eye_body is [1.80, 0.0, -0.65]
    // X_cam = +Y_b - eye.y = 0.0
    // Y_cam = -Z_b + eye.z = -(-0.65) = +0.65
    // Z_cam = -X_b + eye.x = +1.80
    assert(std::abs(v_cockpit(0, 3) - 0.0f) < 1e-4f);
    assert(std::abs(v_cockpit(1, 3) - (-0.65f)) < 1e-4f);
    assert(std::abs(v_cockpit(2, 3) - 1.80f) < 1e-4f);

    // Infinity view matrix has zero translation
    assert(std::abs(v_inf(0, 3)) < 1e-6f);
    assert(std::abs(v_inf(1, 3)) < 1e-6f);
    assert(std::abs(v_inf(2, 3)) < 1e-6f);

    std::cout << "PASSED (View matrices correctly oriented with zero infinity parallax)\n";
}

int main() {
    std::cout << "=== Camera Rig & Head Dynamics Verification ===\n";
    test_dep_and_equilibrium();
    test_g_load_dynamics();
    test_view_matrices();
    std::cout << "All Camera Rig tests passed successfully!\n\n";
    return 0;
}
