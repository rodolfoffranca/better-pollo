# Better Pollo

Versão atual: **0.1.0**.

Fork do [Apollo](https://github.com/ClassicOldSong/Apollo) com sincronização da biblioteca Steam, melhorias de estabilidade e documentação em português brasileiro.

O Better Pollo transmite a área de trabalho e jogos do seu computador para clientes como o [Artemis](https://github.com/ClassicOldSong/moonlight-android). Ele mantém os recursos do Apollo, incluindo monitor virtual com SudoVDA, permissões por dispositivo e codificação de vídeo por hardware.

Este fork é mantido por [rodolfoffranca](https://github.com/rodolfoffranca). Sua base é o Apollo, derivado do [Sunshine](https://github.com/LizardByte/Sunshine), e a integração Steam foi adaptada do [Vibepollo, de Nonary](https://github.com/Nonary/Vibepollo). Os créditos e o histórico dos projetos de origem são preservados.

## O que este fork acrescenta

- Descoberta de jogos instalados nas bibliotecas locais da Steam.
- Sincronização manual e automática da biblioteca com a lista de aplicativos do Apollo.
- Seleção entre todos os jogos instalados ou apenas os jogos recentes.
- Filtros de exclusão e opção para incluir ferramentas da Steam.
- Importação e preparação de capas compatíveis com os clientes.
- Acompanhamento dos processos dos jogos Steam para gerenciar o fim da sessão.
- Opção global na aba Steam para passar a resolução da sessão aos jogos, com detecção automática de Unity, Unreal e Source ou seleção manual do perfil.
- Atualização da biblioteca sem substituir o gerenciador de processos nem apagar o estado da sessão e do monitor virtual.
- Proteção de acessos simultâneos à biblioteca e ao ciclo de execução dos aplicativos.
- Empacotamento completo do SudoVDA, com verificação dos arquivos necessários antes da instalação do driver.
- Encerramento sem tentativas repetidas de reabrir o driver durante a limpeza da sessão.

As alterações deste fork foram compiladas e verificadas em Windows x64. A base mantém código para outras plataformas, mas essas alterações ainda não foram validadas nelas. O monitor virtual descrito aqui utiliza SudoVDA no Windows.

## Começar a usar

1. Instale uma compilação deste fork no computador que executará os jogos. No Windows, execute o instalador como administrador para permitir a instalação do serviço e do SudoVDA.
2. Abra a interface de configuração em [https://localhost:47990](https://localhost:47990), considerando a porta padrão, e crie suas credenciais locais.
3. Abra o Artemis no dispositivo cliente e faça o pareamento com o computador.
4. Confira as permissões do cliente na interface web, incluindo iniciar aplicativos e usar teclado, mouse e controle, conforme necessário.
5. Configure a integração Steam e atualize a lista de aplicativos no cliente.

O nome Apollo ainda aparece no instalador, no serviço e em partes da interface. Better Pollo identifica este fork; a identidade interna do aplicativo foi preservada nesta etapa.

Para obter o código, use este repositório. Consulte a página de [Releases do Better Pollo](https://github.com/rodolfoffranca/better-pollo/releases) para verificar a disponibilidade de binários. Os instaladores do Apollo original não incluem as alterações específicas deste fork.

## Sincronização da Steam

Com a Steam e os jogos instalados no computador host:

1. Acesse **Configurações → Steam** na interface web.
2. Ative **Habilitar Integração com a Steam**.
3. Para acompanhar mudanças automaticamente, ative **Sincronização Automática em Segundo Plano**. A verificação ocorre a cada 30 segundos.
4. Ative **Sincronizar Todos os Jogos Instalados** ou configure a quantidade e a idade máxima dos jogos recentes.
5. Salve as configurações antes de iniciar uma sincronização manual.
6. Use **Sincronizar Biblioteca Steam Agora**, na aba Steam, ou **Sincronizar Steam**, na página de aplicativos.
7. Atualize a lista de aplicativos no cliente para visualizar os jogos importados.

A descoberta usa a instalação e os arquivos locais da Steam. Não é necessário informar a senha da conta Steam ao Better Pollo. O launcher continua responsável pela autenticação e pela inicialização dos jogos. As capas podem vir do cache local ou ser obtidas pela rede.

A remoção automática atua sobre entradas gerenciadas pela sincronização; os atalhos adicionados manualmente mantêm suas configurações de inicialização. No modo de jogos recentes, um título pode sair da lista quando deixa de atender aos filtros, mesmo continuando instalado.

## Monitor virtual e SudoVDA

### Forçar a resolução nos jogos Steam

No Windows, acesse **Configurações → Steam**, habilite **Forçar resolução da sessão nos jogos Steam**, mantenha **Automático por motor** e salve. A opção é geral: não exige configurar cada jogo ou sincronizar a biblioteca novamente. Ela vale para os jogos com identificação Steam iniciados pelo Better Pollo.

Em cada nova inicialização, o Better Pollo utiliza a resolução efetiva da sessão, incluindo a escala configurada, e envia os argumentos ao jogo por meio de `steam.exe -applaunch`. As opções salvas na Steam não são modificadas. Uma sessão de 2560 × 1600, por exemplo, recebe os seguintes argumentos conforme o motor:

| Perfil | Argumentos |
| --- | --- |
| Unity | `-screen-width 2560 -screen-height 1600` |
| Unreal Engine | `-ResX=2560 -ResY=1600 -ForceRes` |
| Source / Source 2 | `-w 2560 -h 1600` |

O modo automático identifica o motor pelos arquivos instalados. Se não conseguir reconhecê-lo, mantém o comando de inicialização e informa isso no log. Um perfil manual aplica os mesmos argumentos a todos os jogos Steam, então escolha essa alternativa apenas quando ela for adequada à sua biblioteca.

O jogo precisa aceitar os argumentos de resolução. Jogos já abertos, iniciados fora do Better Pollo ou que ignoram esses parâmetros não são redimensionados por essa opção. A resolução do monitor virtual continua sendo configurada separadamente pelo Apollo.

### Driver do monitor virtual

O monitor virtual acompanha a resolução e a taxa de atualização solicitadas pelo cliente. Isso configura o monitor da sessão; a resolução interna de cada jogo ainda depende das opções do próprio jogo.

O pacote Windows inclui `SudoVDA.dll`, `nefconc.exe`, o INF, o catálogo e o certificado correspondentes. Os binários foram recuperados, sem modificações, do instalador oficial do Apollo. Sua origem e seus hashes estão documentados em [BINARIES.md](src_assets/windows/drivers/sudovda/BINARIES.md).

Se o monitor virtual não estiver disponível, confira a instalação do SudoVDA e os registros do Apollo. Em uma distribuição portátil, o script `drivers/sudovda/install.bat` precisa ser executado como administrador. Faça uma cópia da pasta `config` antes de atualizar uma instalação existente.

## Compilar no Windows

Use o ambiente **MSYS2 UCRT64**, com o compilador, o CMake, o Ninja, o NSIS e as dependências descritas no [guia de compilação herdado](docs/building.md). Instale o Node.js oficial separadamente e deixe `node` e `npm` disponíveis no `PATH`.

Além das dependências da base, a preparação de capas deste fork utiliza PNG, JPEG e WebP:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-libpng mingw-w64-ucrt-x86_64-libjpeg-turbo mingw-w64-ucrt-x86_64-libwebp
```

Clone o repositório com os submódulos:

```bash
git clone --recurse-submodules https://github.com/rodolfoffranca/better-pollo.git
cd better-pollo
```

Para um clone já existente:

```bash
git submodule update --init --recursive
```

Configure e compile:

```bash
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --parallel 3
```

Na primeira configuração, o CMake pode baixar dependências. A compilação padrão também prepara a interface web.

Gere o instalador Windows:

```bash
cpack --config cmake-build-release/CPackConfig.cmake -G NSIS
```

Os artefatos são gerados em `cmake-build-release/cpack_artifacts`. O executável e o instalador mantêm o nome Apollo. O empacotamento é interrompido se os arquivos obrigatórios do SudoVDA estiverem ausentes.

## Verificação das alterações

Esta versão foi verificada com:

- Compilação de produção para Windows x64 e geração do instalador NSIS.
- Cinco testes de regressão para preservação do estado da sessão, manutenção de uma sessão ativa de entrada, cópias independentes da lista de aplicativos, leituras concorrentes e falhas de gravação.
- Inicialização de uma instância de teste, sincronização real da Steam e 80 consultas autenticadas à biblioteca durante a sincronização.
- Conferência dos arquivos incluídos no instalador e das assinaturas dos binários do driver.
- Validação de uso da instalação corrigida pelo mantenedor deste fork.

Os testes de regressão estão em [test_process_catalog.cpp](tests/unit/test_process_catalog.cpp) e integram a suíte GoogleTest da base, habilitada pela opção CMake `BUILD_TESTS`.

## Próximas melhorias em avaliação

- Sincronização dos jogos instalados da Battle.net.

Essa integração ainda não está implementada nesta versão. A resolução dos jogos Steam é configurada de forma global na aba Steam.

## Atualizações

A verificação automática de atualizações está temporariamente desativada na versão 0.1.0. A interface mostra a versão instalada sem consultar as releases do Apollo original e sem afirmar que esta é a versão mais recente disponível. A opção de notificar pré-lançamentos também foi retirada da interface enquanto a verificação estiver desativada.

## Créditos e projetos de origem

| Projeto | Participação |
| --- | --- |
| [Apollo — ClassicOldSong e colaboradores](https://github.com/ClassicOldSong/Apollo) | Base direta deste fork, recursos de streaming, permissões e integração com monitor virtual. |
| [Sunshine — LizardByte e colaboradores](https://github.com/LizardByte/Sunshine) | Projeto do qual o Apollo deriva e origem de grande parte da infraestrutura de streaming. |
| [Vibepollo — Nonary e colaboradores](https://github.com/Nonary/Vibepollo) | Origem da integração Steam adaptada para este fork. |
| [Artemis — ClassicOldSong e colaboradores](https://github.com/ClassicOldSong/moonlight-android) | Cliente com recursos integrados ao Apollo. |
| SudoVDA — SudoMaker | Driver de monitor virtual utilizado pelo Apollo no Windows. Consulte a [procedência dos binários](src_assets/windows/drivers/sudovda/BINARIES.md). |
| [Moonlight e colaboradores](https://github.com/moonlight-stream) | Ecossistema de clientes de streaming que faz parte da origem do projeto. |

Obrigado aos autores e colaboradores desses projetos. O Better Pollo é mantido de forma independente e não é uma distribuição oficial do Apollo, Sunshine ou Vibepollo. As marcas citadas pertencem aos respectivos titulares.

O [README original do Apollo](docs/README.apollo.md) foi preservado para consulta, incluindo os créditos, links e orientações da base.

## Documentação e contribuições

- [Documentação da base Sunshine](https://docs.lizardbyte.dev/projects/sunshine)
- [Wiki do Apollo](https://github.com/ClassicOldSong/Apollo/wiki)
- [Permissões dos dispositivos](https://github.com/ClassicOldSong/Apollo/wiki/Permission-System)
- [Guia de contribuição](.github/CONTRIBUTING.md)
- [Issues deste fork](https://github.com/rodolfoffranca/better-pollo/issues)

Ao relatar um problema neste fork, inclua a versão ou o commit utilizado, o sistema operacional, a GPU, o cliente e os registros relevantes. Remova informações pessoais dos registros antes de publicá-los.

## Licença

O código do projeto mantém a licença **GNU GPL v3** da base. Consulte [LICENSE](LICENSE) e [NOTICE](NOTICE). Componentes e dependências de terceiros mantêm suas próprias licenças e avisos.
