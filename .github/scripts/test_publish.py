import unittest
from publish import validate


class ReleaseValidation(unittest.TestCase):
    def setUp(self):
        self.builds = [{"version": "1.1.0", "commit": "abc", "platform": platform}
                       for platform in ("Windows-x64", "macOS-universal")]

    def test_accept_matching_tagged_builds(self):
        self.assertEqual(validate("refs/tags/v1.1.0", self.builds), "v1.1.0")

    def test_reject_branch_and_mismatched_version(self):
        for ref in ("main", "v1.2.0", "v1.1.0-beta"):
            with self.assertRaises(ValueError):
                validate(ref, self.builds)

    def test_reject_partial_build_and_mixed_commits(self):
        with self.assertRaises(ValueError):
            validate("v1.1.0", self.builds[:1])
        self.builds[1]["commit"] = "different"
        with self.assertRaises(ValueError):
            validate("v1.1.0", self.builds)


if __name__ == "__main__":
    unittest.main()
