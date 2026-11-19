#include <Project.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <system_error>

namespace Quasar::Editor
{
    namespace
    {
        class ProjectTests : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
                m_Root = std::filesystem::temp_directory_path() / ("QuasarProjectTests-" + testName);
                std::filesystem::remove_all(m_Root);
                std::filesystem::create_directories(m_Root);
            }

            void TearDown() override
            {
                std::error_code error;
                std::filesystem::remove_all(m_Root, error);
            }

            const std::filesystem::path& Root() const { return m_Root; }

        private:
            std::filesystem::path m_Root;
        };
    }

    TEST_F(ProjectTests, GetAssetsDirectoryAppendsAssetsFolderToProjectFolder)
    {
        EXPECT_EQ(GetAssetsDirectory(Root()), Root() / "Assets");
    }

    TEST_F(ProjectTests, CreateProjectCreatesProjectFolderWithAssetsFolder)
    {
        const ProjectResult result = CreateProject(Root(), "MyGame");

        ASSERT_TRUE(result.IsValid()) << result.Error;
        EXPECT_EQ(result.Directory, std::filesystem::absolute(Root() / "MyGame"));
        EXPECT_TRUE(std::filesystem::is_directory(Root() / "MyGame" / "Assets"));
    }

    TEST_F(ProjectTests, CreateProjectKeepsUtf8CharactersOfName)
    {
        const std::string name = "Proj\xC3\xA9t";

        const ProjectResult result = CreateProject(Root(), name);

        ASSERT_TRUE(result.IsValid()) << result.Error;
        EXPECT_EQ(PathToUtf8(result.Directory.filename()), name);
    }

    TEST_F(ProjectTests, CreateProjectFailsWhenNameIsEmpty)
    {
        EXPECT_FALSE(CreateProject(Root(), "").IsValid());
    }

    TEST_F(ProjectTests, CreateProjectFailsWhenNameContainsPathSeparator)
    {
        EXPECT_FALSE(CreateProject(Root(), "My/Game").IsValid());
        EXPECT_FALSE(CreateProject(Root(), "My\\Game").IsValid());
        EXPECT_FALSE(std::filesystem::exists(Root() / "My"));
    }

    TEST_F(ProjectTests, CreateProjectFailsWhenProjectFolderAlreadyExists)
    {
        std::filesystem::create_directory(Root() / "MyGame");

        EXPECT_FALSE(CreateProject(Root(), "MyGame").IsValid());
        EXPECT_FALSE(std::filesystem::exists(Root() / "MyGame" / "Assets"));
    }

    TEST_F(ProjectTests, CreateProjectFailsWhenParentFolderDoesNotExist)
    {
        EXPECT_FALSE(CreateProject(Root() / "Missing", "MyGame").IsValid());
    }

    TEST_F(ProjectTests, OpenProjectReturnsAbsoluteFolderWhenItHasAssetsFolder)
    {
        std::filesystem::create_directories(Root() / "MyGame" / "Assets");

        const ProjectResult result = OpenProject(Root() / "MyGame");

        ASSERT_TRUE(result.IsValid()) << result.Error;
        EXPECT_EQ(result.Directory, std::filesystem::absolute(Root() / "MyGame"));
    }

    TEST_F(ProjectTests, OpenProjectFailsWhenFolderHasNoAssetsFolder)
    {
        std::filesystem::create_directory(Root() / "MyGame");

        EXPECT_FALSE(OpenProject(Root() / "MyGame").IsValid());
    }

    TEST_F(ProjectTests, OpenProjectFailsWhenFolderDoesNotExist)
    {
        EXPECT_FALSE(OpenProject(Root() / "Missing").IsValid());
    }
}
