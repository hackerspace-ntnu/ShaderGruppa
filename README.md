# ShaderGruppa

Dette er et simpelt OpenGL prosjekt som lar deg kjøre compute-shadere skrevet i GLSL,
nedenfor er korte forklaringer på hva hver .cpp fil og deres funksjoner gjør,
og litt om hva de forskjellige shader-typene er og hvordan de brukes i prosjektet

## Kom i gang

### Du trenger

- **Git** og **CMake 3.20+**
- En **C++17**-kompilator (Visual Studio 2019+, CLion (nyeste), GCC 9+, Clang 10+)
- Et skjermkort med **OpenGL 4.3+**. Alt fra ca. 2013 og nyere duger, integrert Intel-grafikk holder som regel.
- Nettilgang første gang du kjører CMake: GLFW lastes ned automatisk

**Windows:** installer Visual Studio med arbeidslasten "Desktop development with C++". CMake følger med.

**Linux (Ubuntu/Debian):**

```
sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev
```

**macOS:** virker ikke. Bruk en maskin/emulator med Windows eller Linux.

### Bygg

```
git clone https://github.com/hackerspace-ntnu/ShaderGruppa.git
cd ShaderGruppa
cmake -S . -B build
cmake --build build
```

Første gang tar det et par minutter, siden GLFW bygges fra kildekode.

### Kjør

Programmet må startes fra mappa der binærfila havnet. CMake kopierer `shaders/` og `assets/` dit, og programmet leter etter dem via relative stier.

Linux:

```
cd build
./DiffusionSim
```

Windows:

```
cd build\Debug
.\DiffusionSim.exe
```

Start det fra et terminalvindu. Shader-menyen kjører i konsollen, ikke i grafikkvinduet.

### Kontroller

- Ved oppstart velger du shader i konsollen: skriv tallet og trykk Enter
- `M` i grafikkvinduet bytter shader. Bildet fryser, du klikker over i konsollvinduet og velger ny shader, og trykker deretter **SPACE i grafikkvinduet** for å fortsette
- Venstre musknapp + dra roterer (for 3D-shaderne)
- Lukk vinduet for å avslutte

### Når noe ikke virker

- **"Failed to create window"**. Driveren gir deg ikke OpenGL 4.3. Oppdater grafikkdriveren. På laptop med to skjermkort må du sørge for at programmet kjører på det dedikerte.
- **CMake feiler under nedlasting av GLFW**. Du er offline, eller bak en brannmur/proxy.
- **En ny `.comp`-fil dukker ikke opp i menyen**. Shaderne kopieres i et POST_BUILD-steg som bare kjøres når det faktisk bygges noe. Bygg på nytt med `cmake --build build --clean-first`, eller kopier fila manuelt inn i mappa ved siden av binærfila.
- **"No .comp files found"**. Du kjører binærfila fra feil mappe. Se "Kjør" over.

## Filoversikt

- `src/main.cpp`  
  Hovedinn- og utgangspunkt:
  - Initialiserer GLFW og GL loader, oppretter GL-kontekst og hovedløkke.
  - Oppretter og endrer størrelse på render-teksturen med `createTextureRGBA8` / `resizeTextureRGBA8`.
  - Laster compute- og blit-program med `buildComputeProgramFromFile` og `buildProgramFromFiles`.
  - Kjører hovedløkken: setter uniforms (`uTime`, `dt`), binder SSBO/tekstur, dispatche compute, barrier, og blit til skjerm.
  - dette programmet trengs ikke endres på for å kjøre nye shaders.

- `src/gl_texture.cpp`  
  Tekstur-hjelpere:
  - `createTextureRGBA8(int w, int h)`, oppretter en `GL_RGBA8` 2D-tekstur med fornuftige parametre.
  - `resizeTextureRGBA8(GLuint tex, int w, int h)`, reallokerer teksturlagring ved vindusstørrelsesendring.
  Hvordan bruke: kall `createTextureRGBA8` med ønsket størrelse.
	- For compute-skriving: bind med `glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8)`
	-  For sampling i fragment shader: bind til en teksturenhet og sett sampler-uniform.

- `src/file_io.cpp`  
  Enkel fil-leser:
  - `readTextFile(const char* path)`, leser hele filen til en `std::string`, kaster `std::runtime_error` ved feil.
  Hvordan bruke: bruk for å hente GLSL-kilde før kompilering, f.eks. i shader-byggere.

- `src/gl_shader.cpp`  
  Shader-kompilering og linking:
  - `compileShader(GLenum type, const char* source, const char* label)`, kompilerer shader og skriver logg ved feil.
  - `linkProgram(const std::vector<GLuint>& shaders, const char* label)`, linker shaders til et program.
  - `buildProgramFromFiles(const char* vertPath, const char* fragPath)`, leser filer og returnerer linket program.
  - `buildComputeProgramFromFile(const char* compPath)`, leser og bygger compute-program.
  Hvordan det brukes av main: kaller `buildComputeProgramFromFile("shaders/foo.comp")` for compute. 
	- For blit brukes `buildProgramFromFiles("shaders/fullscreen.vert", "shaders/blit.frag")`. 
	- Funksjonene kaster ved feil og skriver kompilerings-/link-logg til stderr.

- `src/menu.cpp`  


- `src/pingpong.cpp`  


## Hvordan bruke shaders

Konvensjoner i prosjektet:
- Compute-shadere skriver til en output-tekstur bundet som bilde. Nye shadere bruker `layout(rgba8, binding = 1) uniform writeonly image2D outputImage;`. Eldre shadere bruker navnet `img` på binding 0, og støttes fortsatt.
- Prosjektet antar vanligvis `local_size_x/y = 16`.

- Compute-shadere (`.comp`)  
  - Beregnet for å skrive direkte til en output-tekstur bundet som et bilde.
  - Vanlige uniforms: `uTime` (float), `dt` (float). Threadgroup-størrelse i koden er vanligvis delt i 16×16 (dispatche beregnet fra teksturstørrelse).
  - Navnekonvensjon (prosjektet): render-/update-par navngis `navn_render.comp` og `navn_update.comp`. Bare `_render`-fila vises i menyen; `_update` plukkes opp automatisk.
  - Hvordan bruke: skriv en compute som tar `outputImage` og eventuelle uniforms,
	- kall `glDispatchCompute` med grid beregnet fra output-størrelsen og sørg for `glMemoryBarrier` før tekstursampling.
	- for å legge til en ny compute-shader:
  1. Legg til en .comp-fil i shaders/.
  2. Sørg for `layout(rgba8, binding=1)` output.
  3. (Valgfritt) bruk uTime og dt.
	- For simple shadere trenger man kun lage en compute-shader (er enda usikker på når vi må lage nye vertex/fragment shadere).

- Vertex-shadere (`.vert`)  
  - Brukes her for et enkelt fullscreen-blit. Vertex-shaderen trenger ingen input-attributter; den genererer et fullskjerms-trekantoppsett for fragment-trinnet.
  Hvordan bruke: bygg sammen med fragment-shader for blit/presentasjon.

- Fragment-shadere (`.frag`)  
  - Fragment-shaderen for blit leser fra en `sampler2D` (vanligvis uniform `uTex`) og skriver farge til skjermen.
  Hvordan bruke: bind output-teksturen til en teksturenhet (`GL_TEXTURE0`) og sett `uTex` til 0 før tegning.

## Kort om feilsøking

- Shader-kompilering/link-feil logges til stderr med innhold fra GL-info-loggene.
- `readTextFile` kaster hvis shaderfilen ikke finnes, pass på at relative stier (`shaders/`) er korrekte.

## Litt intuisjon om shader-typer

 - Vertex shadere prosseserer hver vertex (hjørne eller punkt i et mesh) og bestemmer hvor det skal tegnes på skjermen.
	- Vertex shaderen gir en relativ posisjonsvektor for hvert vertex, og kan også sende data videre til fragment shaderen,
	- dette kalles ofte uv-koordinater.

 - Fragment shadere prosserer hvert fragment av et bilde (som vanligvis tilsvarer piksler) og bestemmer fargen på det fragmentet

 - Compute shadere er veldig generelle og lever utenfor grafikk-pipelinen.
	- de brukes i praksis til å gjøre alle slags beregninger på GPUen, og sender så resultatet til en tekstur eller buffer som kan brukes senere i grafikk-pipelinen,
	- resultatet leses ofte av fragment shadere (og noen ganger vertex shadere) for å bestemme hvordan ting skal tegnes på skjermen.
	- et er også vanlig å bruke flere compute shadere i en serie for å gjøre mer komplekse beregninger,
	- der hver shader tar resultatet fra den forrige som input, denne teknikken kalles ofte "ping-pong",
	- og kan også brukes for å lagre data om mellom frames av simulasjoner eller animasjoner.
