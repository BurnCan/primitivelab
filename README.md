# Mesh architecture

Mesh-based objects follow a single pipeline. A strongly typed `MeshSource`
describes procedural geometry, `MeshFactory` creates retained CPU-side
`MeshData`, and move-only `Mesh` owns the uploaded OpenGL buffers.
`MeshObject` combines those pieces with texture and collision state. `Terrain`
keeps its terrain-specific configuration while delegating geometry creation and
rendering through the same pipeline.

Imported models are the intended next `MeshSource` alternative. A model source
and its real asset-loader visitor can be added to `MeshFactory` without changing
`Mesh` or `MeshObject`; no placeholder source is exposed before that loader
exists.

# Dependencies

PrimitiveLab requires:

- A C++17-compatible compiler (GCC 9+, Clang 10+, or MSVC 2022+ recommended)
- CMake 3.16+
- Git

> **Note:** PrimitiveLab uses CMake's `FetchContent` module to automatically download and build third-party libraries during configuration. You **do not** need to manually install libraries such as GLFW, GLAD, or GLM.

---

# OS-Specific Setup

## 🪟 Windows (MSYS2 UCRT64)

1. Download and install the latest 64-bit version of MSYS2.
2. Open the **MSYS2 UCRT64** terminal.
3. Update MSYS2:

```bash
pacman -Syu
```

If prompted, restart the terminal and run:

```bash
pacman -Su
```

4. Install the required development tools:

```bash
pacman -S \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-cmake \
    git
```

That's it! PrimitiveLab downloads all required third-party libraries automatically during the CMake configuration step.

---

## 🍏 macOS

Install Homebrew:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the post-install instructions displayed by Homebrew to add it to your shell environment.

Then install the required tools:

```bash
brew install cmake git
```

PrimitiveLab downloads all required third-party libraries automatically during the CMake configuration step.

---

## 🐧 Ubuntu / Debian

Update your package list and install the required development tools:

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    git \
    libgl1-mesa-dev \
    libx11-dev \
    libxi-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev
```

PrimitiveLab downloads all required third-party libraries automatically during the CMake configuration step.

---

# Building PrimitiveLab

Clone the repository:

```bash
git clone https://github.com/burncan/primitivelab.git
cd primitivelab
```

Alternatively:

💡 To clone a specific branch (e.g., development):
```bash
git clone --branch development --single-branch https://github.com/BurnCan/primitivelab
```

Configure the project:

```bash
cmake -S . -B build
```

During this step, CMake automatically downloads and builds any required third-party dependencies using `FetchContent`.

Build the project:

```bash
cmake --build build
```

---

# Running

## FPS controls

- **W/A/S/D:** Move along the ground
- **Mouse:** Look around
- **Space:** Jump
- **Left or right Ctrl:** Crouch

## Game

From the project root:

```bash
./build/Game
```

Or from inside the `build` directory:

```bash
./Game
```

## Editor

From the project root:

```bash
./build/Editor
```

Or from inside the `build` directory:

```bash
./Editor
```

## Shader assets and render modes

Renderable scene objects can independently use the texture pipeline or a discovered material. Texture shaders live in `shaders/texture`, internal renderer shaders and the shared material vertex shader live in `shaders/engine`, and selectable fragment shaders live directly in `shaders/material`. Adding a `.frag` file to `shaders/material` makes it available at the next application start without C++ registration. Material IDs are lowercase filename stems.

The included `cloudy_sky` material uses the object-space direction supplied by the
shared material vertex shader to render seamless procedural clouds. Select the
skybox in the editor, change its render mode to **Material**, and choose
`cloudy_sky` from the material list.

## Skybox assets and scenes

Named skyboxes live in `textures/skyboxes/<skybox-name>/`. The directory name is the
logical skybox name selected in the editor and stored in scene files. Each directory
must contain all six lowercase face names: `right`, `left`, `top`, `bottom`, `front`,
and `back`. A face may use a lowercase `.png`, `.jpg`, or `.jpeg` extension, and the
extensions may be mixed within one skybox. For example:

```text
textures/
└── skyboxes/
    └── sky1/
        ├── right.jpg
        ├── left.jpg
        ├── top.png
        ├── bottom.jpeg
        ├── front.jpg
        └── back.jpg
```

Both face names and extensions are case-sensitive and must be lowercase. More than
one supported file for a face (for example, both `right.png` and `right.jpg`) makes
the skybox invalid. All six decoded images must also have matching dimensions and
channel counts, and each image must have three or four channels.

The editor discovers complete, valid skyboxes once at startup. Use **Refresh
Skyboxes** to rescan after changing the asset directories; discovery is not performed
every frame. Invalid or incomplete directories are ignored and reported through
console warnings, while image loading failures are shown as loader errors. A scene
stores only the portable logical name, rather than individual image paths or absolute
filesystem paths:

```text
Skybox sky1 0 0 0 RenderMode Texture Material default
```

# Adding a New Built-In Mesh

This procedure adds a mesh that is compiled directly into the application. The mesh geometry is supplied manually through vertex and index arrays, registered as a `MeshSource`, exposed in the editor, and supported by scene saving and loading. The walkthrough uses `Rock` and `RockMeshSource`; replace both names with names appropriate for your mesh. (The repository already contains a Rock implementation, so its current code is also a useful concrete reference.)

## 1. Understand the vertex format

The flattened arrays used by the existing mesh-generation functions contain exactly eight floats per vertex:

```cpp
// positions          // normals           // texcoords
x, y, z,              nx, ny, nz,          u, v
```

This is converted to the structure already defined in `src/Mesh.h`:

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};
```

The first three values become `position`, the next three become `normal`, and the last two become `texCoord`.

### Positions

* `x`, `y`, and `z` are local-space coordinates; the scene object's transform is applied later.
* Centering geometry around the local origin usually makes editor positioning, rotation, scaling, and collision behavior easier to understand.
* A value such as `0.5f` represents half a local unit.

### Normals

* `nx`, `ny`, and `nz` describe the direction the surface faces and are used by lighting calculations.
* Normals should normally be unit-length vectors.
* Common face normals are:

```cpp
0,  1,  0  // upward
0, -1,  0  // downward
1,  0,  0  // +X
-1, 0,  0  // -X
0,  0,  1  // +Z
0,  0, -1  // -Z
```

One vertex stores only one normal. A position shared by several hard-edged faces may therefore need to be duplicated so each face can use a different normal. Incorrect normals can produce incorrect lighting, dark faces, lighting from the wrong direction, or inconsistent shading across hard edges.

A conventional triangle-normal calculation is:

```cpp
glm::vec3 edge1 = p1 - p0;
glm::vec3 edge2 = p2 - p0;
glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
```

Keep triangle winding and cross-product order consistent, because both affect the resulting direction.

### Texture coordinates

* `u` and `v` map a vertex to a position on a 2D texture; `u` normally moves horizontally and `v` vertically.
* Values from `0.0f` to `1.0f` usually cover a texture once.
* Values outside that range may repeat because mesh textures configure both wrap axes with `GL_REPEAT`.

A common four-corner layout is:

```cpp
0.0f, 0.0f
1.0f, 0.0f
1.0f, 1.0f
0.0f, 1.0f
```

The project flips texture images vertically during loading. Compare existing built-in mesh UV layouts when matching texture orientation.

### `TypedMeshData()`

`TypedMeshData()` in `src/MeshFactory.cpp` reads the flattened float array in groups of eight and converts each group into one `Vertex`. Every vertex must contain exactly eight floats. Its loop requires eight available values, so an incomplete final vertex may be skipped; inserting or omitting a value earlier can also shift the grouping and make positions, normals, and UVs be interpreted as the wrong fields.

## 2. Understand the index format

The index array contains zero-based references into the vertex array. Every three indices define a triangle because `Mesh::Draw()` uses indexed `GL_TRIANGLES` rendering:

```cpp
std::vector<unsigned int> indices = {
    0, 1, 2,
    2, 3, 0
};
```

This creates two triangles from four vertices. Every index must be smaller than the vertex count, and the index count should be divisible by three. Keep winding consistent: reversing two indices reverses a triangle's direction, can reverse a normal calculated from it, and may make the face disappear if face culling is enabled.

A complete one-triangle input looks like this:

```cpp
std::vector<float> localVertices = {
    // positions          // normals           // texcoords
    -0.5f, 0.0f, 0.0f,   0, 0, 1,             0.0f, 0.0f,
     0.5f, 0.0f, 0.0f,   0, 0, 1,             1.0f, 0.0f,
     0.0f, 1.0f, 0.0f,   0, 0, 1,             0.5f, 1.0f
};

std::vector<unsigned int> indices = {
    0, 1, 2
};
```

## 3. Add the mesh source structure in `src/MeshSource.h`

The first code change is a source type beside the existing mesh source structures. For a fixed mesh, add:

```cpp
struct RockMeshSource {};
```

Any configurable values belong directly in this structure, because it stores mesh source configuration. For example:

```cpp
struct RockMeshSource {
    float width = 1.0f;
    float height = 1.0f;
};
```

The structure stores the identity and configuration needed to regenerate the mesh. Add it to `MeshSource`, preserving the surrounding order and formatting as closely as practical:

```cpp
using MeshSource = std::variant<
    TriangleMeshSource,
    PlaneMeshSource,
    CubeMeshSource,
    SlabMeshSource,
    TriangularPrismMeshSource,
    SphereMeshSource,
    RockMeshSource,
    TerrainMeshSource
>;
```

Then declare the fixed helper inside `namespace MeshSources`:

```cpp
MeshSource Rock();
```

For the configurable form, declare the parameters instead:

```cpp
MeshSource Rock(
    float width = 1.0f,
    float height = 1.0f
);
```

The source is later stored by `MeshObject`, passed into `MeshFactory::Create()`, used to identify the mesh, used by saving and loading, and available to preserve configuration values.

## 4. Declare the mesh-generation function in `src/MeshFactory.cpp`

Near the top of `src/MeshFactory.cpp`, add the declaration inside the existing `MeshGeneration` declarations:

```cpp
namespace MeshGeneration {
MeshData CreateTriangle();
MeshData CreatePlane();
MeshData CreateCube();
MeshData CreateSlab();
MeshData CreateTriangularPrism();
MeshData CreateSphere(unsigned int, unsigned int);
MeshData CreateRock();
}
```

This lets the factory dispatch code later in the file call the mesh-generation function.

## 5. Implement `MeshGeneration::CreateRock()`

Add the implementation in `src/MeshFactory.cpp`:

```cpp
MeshData MeshGeneration::CreateRock() {
    std::vector<float> localVertices = {
        // positions          // normals           // texcoords
        // Paste the user-provided vertex data here.
    };

    std::vector<unsigned int> indices = {
        // Paste the user-provided index data here.
    };

    return TypedMeshData(localVertices, std::move(indices));
}
```

Replace the comments with the actual vertex and index arrays, then check:

* Each vertex contains exactly eight floats.
* The layout is position, normal, then texture coordinate.
* All indices refer to valid vertices.
* The index count is divisible by three.
* Triangle winding is consistent.
* Normals point in the intended direction.
* Hard-edged faces duplicate vertices where separate normals are needed.
* UV coordinates map the texture as intended.
* The mesh is positioned around a sensible local origin.

Returning manually constructed `MeshData` containing `Vertex` objects is also valid, but the flattened float-array style matches the current built-in mesh implementations. Do not introduce a new vertex format.

## 6. Define `MeshSources::Rock()`

Beside the existing `MeshSources` functions in `src/MeshFactory.cpp`, define the fixed helper:

```cpp
MeshSource MeshSources::Rock() {
    return RockMeshSource{};
}
```

It constructs the source passed to `SceneManager::AddMesh()`. For a configurable source:

```cpp
MeshSource MeshSources::Rock(float width, float height) {
    return RockMeshSource{width, height};
}
```

The argument order must match the fields in `RockMeshSource`.

## 7. Register the mesh in `MeshFactory::Create()`

`MeshFactory::Create()` uses `std::visit` to inspect the active `MeshSource` variant alternative. Add the Rock branch before the final terrain fallback:

```cpp
else if constexpr (std::is_same_v<T, SphereMeshSource>)
    return MeshGeneration::CreateSphere(
        definition.xSegments,
        definition.ySegments
    );
else if constexpr (std::is_same_v<T, RockMeshSource>)
    return MeshGeneration::CreateRock();
else
    return Terrain::GenerateMeshData(definition);
```

Without the branch, `RockMeshSource` has no geometry-generation path: compilation may fail, or an incorrectly modified visitor could send it into the wrong final branch. For configurable geometry, pass the values from `definition`:

```cpp
else if constexpr (std::is_same_v<T, RockMeshSource>)
    return MeshGeneration::CreateRock(
        definition.width,
        definition.height
    );
```

The corresponding declaration and definition of `CreateRock` must accept those parameters too.

## 8. Add the mesh display name

In `MeshObject::GetSourceName()` in `src/MeshFactory.cpp`, add:

```cpp
else if constexpr (std::is_same_v<T, RockMeshSource>)
    return "Rock";
```

This supplies the default scene object name, the mesh type displayed by the editor, and the base mesh token written during saving. `SceneManager::saveScene()` removes spaces from mesh names, so the loader token must match the serialized form. For example, `Triangular Prism` is saved as `TriangularPrism`; `Rock` remains `Rock`.

## 9. Add the editor creation enum entry

Add `Rock` to `enum class CreationOption` in `src/editor.cpp`:

```cpp
enum class CreationOption {
    Triangle,
    Plane,
    Cube,
    TriangularPrism,
    Sphere,
    Slab,
    Rock,
    Terrain,
    Skybox,
    Light
};
```

`CreationOption::Rock` identifies an editor menu choice; `RockMeshSource` identifies the actual mesh source. They are separate types with separate purposes.

## 10. Add the mesh to an existing editor category

A rock is a 3D mesh, so it can be registered in `primitive3DTypes` (the current repository places Rock under Environment, but this example demonstrates adding it to the existing `Primitives 3D` category). Use the repository's literal-count style:

```cpp
static const CreationOptionOption primitive3DTypes[] = {
    {CreationOption::Cube, "Cube"},
    {CreationOption::TriangularPrism, "Triangular Prism"},
    {CreationOption::Sphere, "Sphere"},
    {CreationOption::Slab, "Slab"},
    {CreationOption::Rock, "Rock"}
};
```

Update the corresponding literal count:

```cpp
CreationOptionGroup(
    "Primitives 3D",
    primitive3DTypes,
    5,
    selectedCreationOption
);
```

Keep the literal count style currently used by the repository. Do not replace it with `std::size`, introduce a template overload, or refactor the category system as part of adding a mesh. If the array has five entries while the literal remains `4`, Rock will not appear. The other current category label is `Primitives 2D`.

## 11. Register the Add-button switch case

Inside the Add button's `switch (selectedCreationOption)`, add:

```cpp
case CreationOption::Rock:
    sceneManager.AddMesh(
        MeshSources::Rock(),
        creationTexture
    );
    break;
```

> **Warning:** This is incorrect:
>
> ```cpp
> sceneManager.AddRock();
> ```

`SceneManager` has no dedicated `AddRock()` method. Ordinary built-in meshes use the generic path:

```cpp
sceneManager.AddMesh(
    MeshSources::Rock(),
    creationTexture
);
```

`MeshSources::Rock()` supplies the mesh source, `creationTexture` supplies the currently selected texture, and `SceneManager::AddMesh()` constructs the `SceneObject` and its `MeshObject`. This avoids:

```text
'class SceneManager' has no member named 'AddRock'
```

## 12. Adding a new editor category

Instead of placing Rock in `Primitives 3D`, a new `Natural` category can organize it.

### Define the category array

```cpp
static const CreationOptionOption naturalTypes[] = {
    {CreationOption::Rock, "Rock"}
};
```

### Include it in selected-name lookup

The editor checks each category array to choose the combo preview label. Add:

```cpp
for (const CreationOptionOption& option : naturalTypes)
    if (option.type == selectedCreationOption)
        selectedSceneObjectName = option.name;
```

Without this lookup, the preview may continue to display another item's name.

### Render the new category

Inside the creation combo, add:

```cpp
ImGui::Separator();
CreationOptionGroup(
    "Natural",
    naturalTypes,
    1,
    selectedCreationOption
);
```

Continue using a literal count. A category controls only editor organization; it does not replace the source structure, source variant, mesh-generation function, factory dispatch, display name, creation enum, Add-button switch, or scene-loading work.

## 13. Add fixed-mesh loading support in `src/SceneManager.cpp`

This change belongs inside:

```cpp
bool SceneManager::loadScene(const std::string& path)
```

`src/SceneManager.cpp` currently compresses many statements onto one line. In a checkout that does not yet register Rock, search for this exact text:

```cpp
else if(type=="TriangularPrism")o=&AddMesh(MeshSources::TriangularPrism(),tex);else o=&AddMesh(MeshSources::Cube(),tex);
```

Replace only that portion with:

```cpp
else if(type=="TriangularPrism")o=&AddMesh(MeshSources::TriangularPrism(),tex);else if(type=="Rock")o=&AddMesh(MeshSources::Rock(),tex);else o=&AddMesh(MeshSources::Cube(),tex);
```

The equivalent readable logic is:

```cpp
else if (type == "TriangularPrism") {
    o = &AddMesh(MeshSources::TriangularPrism(), tex);
}
else if (type == "Rock") {
    o = &AddMesh(MeshSources::Rock(), tex);
}
else {
    o = &AddMesh(MeshSources::Cube(), tex);
}
```

Both forms represent the same logic; developers may preserve the current file's compressed style.

> **This is the only required `SceneManager.cpp` change for a fixed, non-configurable Rock mesh.** Do not add `SceneManager::AddRock()` and do not edit `SceneManager.h` for a fixed mesh.

The branch is required because (1) `MeshObject::GetSourceName()` returns `"Rock"`; (2) `SceneManager::saveScene()` writes that as the mesh type token; (3) `loadScene()` reads it into `type`; (4) the loader must recognize it and call `MeshSources::Rock()`; and (5) otherwise the final fallback loads a cube.

## 14. Optional: configurable-mesh serialization

No extra save-side parameter handling is required for a fixed Rock mesh. This subsection applies only when configuration such as the following must survive reopening a scene:

```cpp
struct RockMeshSource {
    float width = 1.0f;
    float height = 1.0f;
};
```

The existing sphere token is the architectural example:

```text
Sphere:xSegments:ySegments
```

A possible Rock token is:

```text
Rock:width:height
```

A configurable implementation must update both saving and loading.

### Saving configurable values

Inside `SceneManager::saveScene()`, after the sphere-specific source check, add logic conceptually similar to:

```cpp
if (const auto* rock =
        std::get_if<RockMeshSource>(&m->GetMeshSource())) {
    n += ":" + std::to_string(rock->width)
       + ":" + std::to_string(rock->height);
}
```

Adapt it to the actual fields in `RockMeshSource`.

### Loading configurable values

Replace the fixed `type == "Rock"` branch with a prefix check:

```cpp
else if (type.rfind("Rock", 0) == 0) {
    float width = 1.0f;
    float height = 1.0f;

    if (type.size() > 4) {
        std::sscanf(
            type.c_str(),
            "Rock:%f:%f",
            &width,
            &height
        );
    }

    o = &AddMesh(
        MeshSources::Rock(width, height),
        tex
    );
}
```

Fixed meshes should not make these unnecessary serialization edits.

## 15. Understand the complete runtime path

```text
CreationOption::Rock
    -> MeshSources::Rock()
    -> RockMeshSource
    -> SceneManager::AddMesh()
    -> MeshObject
    -> MeshFactory::Create()
    -> MeshGeneration::CreateRock()
    -> MeshData
    -> Mesh::Upload()
    -> Mesh::Draw()
```

The editor option selects the helper; the helper constructs the typed source; `AddMesh()` places it in a new scene object; `MeshObject` retains the source and asks the factory for geometry; the visitor dispatches to `CreateRock()`; that function produces CPU-side `MeshData`; and `Mesh` uploads its vertices and indices before drawing them. Each registration step connects one of these transitions.

## 16. What normally does not need to change

A built-in mesh using the current position/normal/UV format normally needs no changes to `src/Mesh.h`, `src/Mesh.cpp`, `src/SceneManager.h`, or `CMakeLists.txt`.

### `src/Mesh.h`

It already defines the required layout:

```cpp
glm::vec3 position;
glm::vec3 normal;
glm::vec2 texCoord;
```

### `src/Mesh.cpp`

It already uploads the `Vertex` array and index array, configures vertex attribute locations 0, 1, and 2, draws indexed triangles, and uses the same retained geometry for mesh collision checks.

### `src/SceneManager.h`

The generic `AddMesh()` already accepts any registered `MeshSource`; a dedicated `AddRock()` is unnecessary.

### `CMakeLists.txt`

The implementation stays in existing source files already included in the common sources compiled into both Editor and Game.

## 17. Troubleshooting

### Compiler error: `SceneManager has no member named AddRock`

Incorrect:

```cpp
sceneManager.AddRock();
```

Correct:

```cpp
sceneManager.AddMesh(
    MeshSources::Rock(),
    creationTexture
);
```

### `Rock` is not a member of `CreationOption`

**Cause:** Rock was used in the option array or switch before being added to `enum class CreationOption`. **Fix:** add it to the enum.

### `MeshSources::Rock` is not declared

**Cause:** its declaration is missing from `namespace MeshSources` in `MeshSource.h`. **Fix:**

```cpp
MeshSource Rock();
```

### Undefined reference to `MeshSources::Rock`

**Cause:** it was declared but not defined in `MeshFactory.cpp`. **Fix:**

```cpp
MeshSource MeshSources::Rock() {
    return RockMeshSource{};
}
```

### Variant or visitor compilation error

**Cause:** `RockMeshSource` was added to `MeshSource` without a matching `MeshFactory::Create()` branch. **Fix:**

```cpp
else if constexpr (std::is_same_v<T, RockMeshSource>)
    return MeshGeneration::CreateRock();
```

Also verify that `MeshObject::GetSourceName()` has the Rock branch.

### Rock does not appear in the dropdown

Check that `CreationOption::Rock` exists, the option is in the intended category array, its literal count was increased, and—when using a new category—that it is rendered with `CreationOptionGroup()` and included in selected-name lookup.

### Rock appears, but pressing Add fails to compile

Check that the switch uses `AddMesh()`, not `AddRock()`; `MeshSources::Rock()` is declared and defined; `RockMeshSource` is in the variant; and the editor includes the headers exposing `MeshSources`.

### Rock is created but nothing is visible

Check that both arrays are nonempty, every index is valid, the object is near the camera/local origin, its scale is nonzero, triangles have nonzero area, and winding is correct.

### Rock renders with broken lighting

Check that normals are correct and normalized, hard edges use duplicated vertices, and normal direction agrees with triangle winding.

### Texture mapping is stretched or rotated

Check UVs, positions duplicated to allow different UVs, existing mesh orientation conventions, and the vertical image flip during texture loading.

### Rock saves but reopens as a cube

**Cause:** the `"Rock"` branch is missing from `SceneManager::loadScene()` before the cube fallback. **Fix:**

```cpp
else if(type=="Rock")o=&AddMesh(MeshSources::Rock(),tex);
```

### Configurable Rock values reset after reopening

The source values were not appended during saving, were not parsed during loading, or the loader called `MeshSources::Rock()` without the saved arguments. Implement both halves of the optional serialization procedure.

## 18. Final verification checklist

### Source registration

* [ ] `RockMeshSource` was declared in `MeshSource.h`.
* [ ] Configuration fields were placed directly in the source structure if needed.
* [ ] `RockMeshSource` was added to `MeshSource`.
* [ ] `MeshSources::Rock()` was declared.

### Geometry

* [ ] `CreateRock()` was declared.
* [ ] `CreateRock()` was implemented.
* [ ] Every vertex contains eight floats.
* [ ] The layout is position, normal, UV.
* [ ] Every index is valid.
* [ ] The index count is divisible by three.
* [ ] Winding, normals, and UVs were checked.

### Factory registration

* [ ] `MeshSources::Rock()` was defined.
* [ ] `MeshFactory::Create()` handles `RockMeshSource`.
* [ ] `MeshObject::GetSourceName()` returns `"Rock"`.

### Editor registration

* [ ] `CreationOption::Rock` was added.
* [ ] Rock was added to a category array.
* [ ] The category's literal count was updated.
* [ ] A new category was included in selected-name lookup if applicable.
* [ ] A new category was rendered if applicable.
* [ ] The Add-button switch calls `sceneManager.AddMesh(...)`.
* [ ] The switch does not call `sceneManager.AddRock()`.

### Scene loading

* [ ] A fixed Rock branch was added inside `SceneManager::loadScene()`.
* [ ] The branch appears before the final cube fallback.
* [ ] Configurable values are saved and parsed only if the mesh is configurable.

### Testing

* [ ] `Editor` compiles.
* [ ] `Game` compiles.
* [ ] Rock appears in the editor dropdown.
* [ ] Pressing Add creates a Rock scene object.
* [ ] The mesh renders correctly from all sides.
* [ ] Lighting appears correct.
* [ ] Texture mapping appears correct.
* [ ] Wireframe topology appears correct.
* [ ] Collision visualization matches the mesh.
* [ ] The scene saves successfully.
* [ ] The saved scene reopens as Rock rather than Cube.
