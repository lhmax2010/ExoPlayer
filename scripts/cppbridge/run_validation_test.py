import unittest

import run_validation


class RunValidationTest(unittest.TestCase):
    def test_has_online_device_accepts_tab_separated_device(self):
        devices = "List of devices attached\nemulator-5554\tdevice\n"

        self.assertTrue(run_validation.has_online_device(devices))

    def test_has_online_device_accepts_space_separated_device(self):
        devices = "List of devices attached\n192.0.2.10:5555 device\n"

        self.assertTrue(run_validation.has_online_device(devices))

    def test_has_online_device_accepts_matching_serial(self):
        devices = "List of devices attached\nemulator-5554\tdevice\nemulator-5556\tdevice\n"

        self.assertTrue(run_validation.has_online_device(devices, serial="emulator-5556"))

    def test_has_online_device_rejects_missing_serial(self):
        devices = "List of devices attached\nemulator-5554\tdevice\n"

        self.assertFalse(run_validation.has_online_device(devices, serial="emulator-5556"))

    def test_has_online_device_rejects_header_offline_and_unauthorized(self):
        devices = (
            "List of devices attached\n"
            "emulator-5554\toffline\n"
            "emulator-5556\tunauthorized\n"
        )

        self.assertFalse(run_validation.has_online_device(devices))

    def test_local_validation_commands_cover_inventory_unit_androidtest_and_demo(self):
        commands = run_validation.local_validation_commands(
            "./gradlew", python_executable="python-test"
        )

        self.assertEqual(
            commands,
            [
                [
                    "python-test",
                    "scripts/cppbridge/api_parity_inventory.py",
                    "--check",
                ],
                [
                    "./gradlew",
                    ":lib-exoplayer-cppbridge:testDebugUnitTest",
                    ":lib-exoplayer-cppbridge:assembleDebugAndroidTest",
                    ":demo-cppbridge:assembleDebug",
                ],
            ],
        )


if __name__ == "__main__":
    unittest.main()
