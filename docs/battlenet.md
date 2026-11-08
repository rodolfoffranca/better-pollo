# Integração Battle.net

A integração é nativa do Better Pollo e está disponível no Windows, em **Configurações → Biblioteca → Battle.net**. Não é necessário instalar Playnite ou outro gerenciador de bibliotecas.

## Configuração

1. Instale a Battle.net e pelo menos um jogo no computador host.
2. Abra o launcher uma vez, conclua o login e as atualizações necessárias.
3. Habilite a integração na seção Battle.net e salve as configurações.
4. Sincronize a biblioteca. A lista mostra os jogos instalados reconhecidos.
5. Atualize a lista de aplicativos no cliente de streaming e selecione um jogo.

A Battle.net pode estar fechada ou já aberta. Se estiver fechada, o Better Pollo a inicia e aguarda sua janela de controle antes de solicitar o jogo. Há uma única repetição da solicitação se o jogo não aparecer após dez segundos; a espera total continua limitada pelo tempo configurado.

| Configuração | Padrão | Comportamento |
| --- | --- | --- |
| `battlenet_enabled` | `disabled` | Habilita a descoberta, sincronização e abertura dos jogos pela integração. |
| `battlenet_auto_sync` | `disabled` | Reconcilia a biblioteca a cada 30 segundos. |
| `battlenet_start_minimized` | `enabled` | Solicita início minimizado quando o launcher estava fechado. |
| `battlenet_remove_uninstalled` | `enabled` | Remove entradas automáticas ausentes em uma leitura completa. |
| `battlenet_launch_timeout` | `60` | Limite de inicialização em segundos, de 15 a 180. |

Não é preciso reiniciar o host para alterar essas opções. A configuração de inicialização é usada na próxima abertura do jogo.

## Descoberta e sincronização

O provedor consulta as entradas de desinstalação do Windows nas visões de 32 e 64 bits e o banco local `%ProgramData%\Battle.net\Agent\product.db`. A descoberta usa o usuário interativo quando o Apollo está rodando como serviço, sem substituir globalmente o Registro do Windows.

Um jogo só é importado quando seu identificador é reconhecido e um executável esperado existe na pasta instalada. Isso evita incluir componentes do launcher, agentes, registros antigos sem arquivos e versões desconhecidas como se fossem jogos prontos para iniciar.

Entre os identificadores reconhecidos estão Hearthstone, World of Warcraft, Diablo III, Diablo IV, Diablo II: Resurrected, Overwatch 2, StarCraft, StarCraft II e Heroes of the Storm. Há também entradas para outros títulos na tabela do provedor. Essa tabela não representa uma garantia de compatibilidade com todas as versões, regiões, betas ou jogos vendidos na Battle.net.

Os IDs dos aplicativos são estáveis entre sincronizações. Entradas Steam e manuais não são reescritas; capas personalizadas e opções de monitor das entradas importadas são preservadas. Esta implementação usa uma capa genérica para novas entradas Battle.net e não consulta serviços de capas.

Se o banco estiver bloqueado, truncado ou ilegível, jogos já importados são preservados. Jogos reconhecidos por outra fonte ainda podem ser adicionados. Se o launcher ou o usuário conectado não estiver disponível, o catálogo existente também é mantido.

## Inicialização, bandeja e encerramento

O host inicia um processo auxiliar na sessão do usuário do Windows. Esse processo valida novamente a instalação, abre o launcher se necessário e envia o comando de inicialização do produto. A espera não bloqueia as threads HTTP ou de streaming.

A opção de minimizar é uma tentativa, não um modo invisível garantido. O Better Pollo solicita inicialização automática/minimizada e respeita a preferência de bandeja da própria Battle.net. Não altera os arquivos de configuração do launcher nem oculta permanentemente suas janelas. Login, atualizações, termos de uso e outros avisos podem aparecer.

O launcher e seus agentes ficam fora do grupo de processos da sessão e são excluídos da associação com o jogo. Ao encerrar uma sessão, o host cancela o processo auxiliar e solicita o encerramento dos processos de jogo que identificou como novos naquela inicialização. Um jogo que já estava aberto pode ser acompanhado pelo auxiliar, mas não é apropriado como processo iniciado pela sessão.

Quando o jogo não inicia dentro do limite, a sessão é encerrada. Confira o registro do host e abra a Battle.net para resolver login ou atualizações antes de tentar novamente. A integração não automatiza autenticação nem contorna exigências do launcher.

## Validação

A integração foi validada no Windows com uma instalação real de Hearthstone: descoberta pelo Registro e banco local, importação com ID estável, abertura com a Battle.net fechada, reutilização do launcher aberto, sincronização durante a execução e encerramento do jogo sem fechar a Battle.net. Também foi conferida a preservação de um Hearthstone iniciado fora do host.

Esses testes usaram a API local de aplicativos, que aciona o mesmo ciclo de execução dos jogos do cliente. A transmissão RTSP por um dispositivo remoto não fez parte dessa verificação. Há 14 testes automatizados específicos de Battle.net, além dos 15 testes de catálogo e resolução Steam, totalizando 29 testes de regressão.

## Referências e créditos

O código do provedor foi implementado em C++ para este fork. Os projetos abaixo serviram como referências de interoperabilidade para o formato local e os identificadores; nenhum deles é uma dependência de execução:

- [PlayniteExtensions: BattleNetGames.cs](https://github.com/JosefNemec/PlayniteExtensions/blob/master/source/Libraries/BattleNetLibrary/BattleNetGames.cs)
- [PlayniteExtensions: BattleNetLibrary.cs](https://github.com/JosefNemec/PlayniteExtensions/blob/master/source/Libraries/BattleNetLibrary/BattleNetLibrary.cs)
- [Lutris: serviço Battle.net](https://github.com/lutris/lutris/blob/master/lutris/services/battlenet.py)

Battle.net e os nomes dos jogos são marcas dos respectivos titulares. O projeto mantém os créditos e a licença do Apollo, Sunshine e demais componentes descritos no README principal.
